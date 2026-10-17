#include "gamesession.h"

#include <QtMath>
#include <cmath>

GameSession::~GameSession()
{
    clear();
}

void GameSession::clear()
{
    // 子弹先销毁，保证任何时候都不会留下指向已销毁敌人的目标引用。
    qDeleteAll(m_bullets);
    m_bullets.clear();
    qDeleteAll(m_enemies);
    m_enemies.clear();
    qDeleteAll(m_towers);
    m_towers.clear();
    m_selectedTower = nullptr;
    m_scheduler = AttackScheduler();
    m_state = State::Stopped;
    m_gold = 150;
    m_life = 10;
    m_nextPathIndex = 0;
    m_bossAuraActive = false;
    m_bossAuraTriggered = false;
}

void GameSession::reset(const AttackPlan& plan, bool running)
{
    clear();
    m_scheduler.reset(plan);
    m_state = running && m_scheduler.isReady() ? State::Running : State::Stopped;
}

bool GameSession::setPaused(bool paused)
{
    if (paused && m_state == State::Running) {
        m_state = State::Paused;
        return true;
    }
    if (!paused && m_state == State::Paused) {
        m_state = State::Running;
        return true;
    }
    return false;
}

void GameSession::spawnEnemy(const QString& type, int waveIndex, QVector<Notice>& notices)
{
    if (m_map.paths().isEmpty()) {
        return;
    }
    const int pathIndex = m_nextPathIndex % static_cast<int>(m_map.paths().size());
    ++m_nextPathIndex;
    Enemy* enemy = new Enemy(type, m_map.paths().at(pathIndex));
    if (waveIndex >= 3) {
        enemy->applyLateGameEnhance();
    }
    if (m_bossAuraActive) {
        enemy->applyBossAura();
    }
    m_enemies.append(enemy);
    if (type == QStringLiteral("boss") && !m_bossAuraTriggered) {
        m_bossAuraTriggered = true;
        m_bossAuraActive = true;
        applyBossAuraToAllEnemies();
        notices.append({QStringLiteral("警报：Boss出现！所有敌人生命提升50%！"), 150});
    }
}

GameSession::FrameResult GameSession::step()
{
    FrameResult result;
    if (m_state != State::Running) {
        return result;
    }
    // 清场观察发生在本 tick 移动和死亡清理之前，不能提前删除对象。
    const auto events = m_scheduler.advance(m_enemies.size());
    if (!m_scheduler.isReady()) {
        m_state = State::Stopped;
        result.stopped = true;
        result.notices.append({QStringLiteral("进攻计时超出可用范围，请退出并重新开始。"), 600});
        return result;
    }
    result.advanced = true;
    for (const auto& event : events) {
        if (event.kind == AttackEventKind::WaveStart) {
            if (event.waveIndex == 3) {
                result.notices.append({QStringLiteral("警报：第4波开始，敌人获得护甲强化！"), 150});
            }
            if (event.waveIndex == 5) {
                result.notices.append({QStringLiteral("最终波开始！Boss 即将出现！"), 150});
                result.finalWaveStarted = true;
            }
        } else {
            spawnEnemy(event.enemyType, event.waveIndex, result.notices);
        }
    }

    for (Enemy* enemy : m_enemies) {
        enemy->update();
    }
    // 所有塔先发射，再统一更新子弹；新弹当 tick 参与移动和伤害。
    for (Tower* tower : m_towers) {
        if (Enemy* target = tower->updateAttack(m_enemies)) {
            m_bullets.append(new Bullet(tower->position(), target, tower->damage()));
        }
    }
    for (Bullet* bullet : m_bullets) {
        bullet->update();
    }
    for (int index = m_bullets.size() - 1; index >= 0; --index) {
        if (m_bullets.at(index)->isFinished()) {
            delete m_bullets.at(index);
            m_bullets.removeAt(index);
        }
    }
    for (int index = m_enemies.size() - 1; index >= 0; --index) {
        Enemy* enemy = m_enemies.at(index);
        // 同时死亡和到终点时仍按漏怪处理，保持既有判定优先级。
        if (enemy->hasReachedEnd()) {
            m_life -= enemy->damageToLife();
            if (enemy->isBoss()) {
                m_bossAuraActive = false;
                removeBossAuraFromAllEnemies();
            }
            removeBulletsTargeting(enemy);
            delete enemy;
            m_enemies.removeAt(index);
        } else if (enemy->isDead()) {
            m_gold += enemy->reward();
            if (enemy->isBoss()) {
                m_bossAuraActive = false;
                removeBossAuraFromAllEnemies();
                result.notices.append({QStringLiteral("Boss已被击败，强化光环消失！"), 120});
            }
            removeBulletsTargeting(enemy);
            delete enemy;
            m_enemies.removeAt(index);
        }
    }
    checkResult();
    return result;
}

GameSession::BuildEvaluation GameSession::evaluateBuild(const QPointF& position,
                                                       TowerType type) const
{
    BuildEvaluation result;
    if (m_state != State::Running || !std::isfinite(position.x())
        || !std::isfinite(position.y()) || position.x() < 0
        || position.x() >= GameMap::Width || position.y() < GameMap::HudHeight
        || position.y() >= GameMap::Height) {
        return result;
    }
    result.center = m_map.snapToGrid(position);
    if (!m_map.circleInsideMap(result.center, Tower::radiusForType(type))) {
        return result;
    }
    if (m_map.isNearRoad(result.center)) {
        result.problem = BuildProblem::Road;
        return result;
    }
    for (const Tower* tower : m_towers) {
        const QPointF difference = result.center - tower->position();
        if (qSqrt(difference.x() * difference.x() + difference.y() * difference.y()) < 42) {
            result.problem = BuildProblem::Occupied;
            return result;
        }
    }
    result.problem = m_gold < Tower::costForType(type)
                         ? BuildProblem::InsufficientGold : BuildProblem::None;
    return result;
}

GameSession::ActionResult GameSession::build(const QPointF& position, TowerType type)
{
    const BuildEvaluation evaluation = evaluateBuild(position, type);
    if (evaluation.problem == BuildProblem::InsufficientGold) {
        return {QStringLiteral("金币不足，无法建造防御塔！"), 60, false};
    }
    if (!evaluation.valid()) {
        return {QStringLiteral("这里不能建造防御塔！"), 60, false};
    }
    m_towers.append(new Tower(evaluation.center, type));
    m_gold -= Tower::costForType(type);
    return {QString(), 60, true};
}

bool GameSession::selectTowerAt(const QPointF& position)
{
    if (m_state != State::Running) {
        return false;
    }
    for (Tower* tower : m_towers) {
        if (tower->containsPoint(position)) {
            m_selectedTower = tower;
            return true;
        }
    }
    return false;
}

void GameSession::clearSelectedTower()
{
    m_selectedTower = nullptr;
}

GameSession::ActionResult GameSession::upgradeSelectedTower()
{
    if (m_state != State::Running) {
        return {};
    }
    if (m_selectedTower == nullptr) {
        return {QStringLiteral("请先点击选择一座防御塔。"), 60, false};
    }
    if (!m_selectedTower->canUpgrade()) {
        return {QStringLiteral("这座防御塔已经满级。"), 60, false};
    }
    const int cost = m_selectedTower->upgradeCost();
    if (m_gold < cost) {
        return {QStringLiteral("金币不足，无法升级。"), 60, false};
    }
    m_gold -= cost;
    m_selectedTower->upgrade();
    return {QString(), 60, true};
}

GameSession::ActionResult GameSession::sellSelectedTower()
{
    if (m_state != State::Running) {
        return {};
    }
    if (m_selectedTower == nullptr) {
        return {QStringLiteral("请先点击选择一座防御塔。"), 60, false};
    }
    m_gold += m_selectedTower->sellValue();
    const int index = m_towers.indexOf(m_selectedTower);
    delete m_selectedTower;
    m_towers.removeAt(index);
    m_selectedTower = nullptr;
    return {QString(), 60, true};
}

void GameSession::removeBulletsTargeting(const Enemy* enemy)
{
    for (int index = m_bullets.size() - 1; index >= 0; --index) {
        if (m_bullets.at(index)->target() == enemy) {
            delete m_bullets.at(index);
            m_bullets.removeAt(index);
        }
    }
}

void GameSession::applyBossAuraToAllEnemies()
{
    for (Enemy* enemy : m_enemies) {
        if (!enemy->isDead() && !enemy->hasReachedEnd()) {
            enemy->applyBossAura();
        }
    }
}

void GameSession::removeBossAuraFromAllEnemies()
{
    for (Enemy* enemy : m_enemies) {
        if (!enemy->isBoss() && !enemy->isDead() && !enemy->hasReachedEnd()) {
            enemy->removeBossAura();
        }
    }
}

void GameSession::checkResult()
{
    if (m_life <= 0) {
        m_state = State::Lost;
    } else if (!m_scheduler.hasPendingGeneration()
               && m_scheduler.spawnedCount() == m_scheduler.totalEnemies()
               && m_enemies.isEmpty()) {
        m_state = State::Won;
    }
}
