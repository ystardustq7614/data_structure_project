/** @file
 * @brief 真实战局回归；夹具只设置触发条件，命令、模拟、伤害和结算均调用产品实现。 */
#include "../26C-/core/attackplanloader.h"
#include "../26C-/core/gamesession.h"

#include <QCoreApplication>

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>

/** 测试专用友元入口；注入对象后交给 GameSession 独占，不向生产接口开放可变容器。 */
class GameSessionProductTestAccess
{
public:
    static Enemy* spawn(GameSession& session, const QString& type, int wave = 0)
    {
        QVector<GameSession::Notice> notices;
        session.spawnEnemy(type, wave, notices);
        return session.m_enemies.last();
    }
    static void addBullet(GameSession& session, const QPointF& start, Enemy* target, int damage)
    {
        session.m_bullets.append(new Bullet(start, target, damage));
    }
    static bool containsEnemy(const GameSession& session, const Enemy* target)
    {
        for (const Enemy* enemy : session.m_enemies) {
            if (enemy == target) return true;
        }
        return false;
    }
    static void setLife(GameSession& session, int life) { session.m_life = life; }
    static void setGold(GameSession& session, int gold) { session.m_gold = gold; }
    static void killAll(GameSession& session)
    {
        for (Enemy* enemy : session.m_enemies) enemy->takeDamage(100000);
    }
};

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void moveToEnd(Enemy& enemy)
{
    for (int tick = 0; tick < 10000 && !enemy.hasReachedEnd(); ++tick) {
        enemy.update();
    }
    require(enemy.hasReachedEnd(), "fixture could not reach the real path endpoint");
}

void checkConstructionAndActions(const AttackPlan& plan)
{
    // 同时检查失败前后的金币、实体和选择，防止“有提示但已扣款”的交易回归。
    GameSession session;
    require(session.state() == GameSession::State::Stopped && !session.step().advanced,
            "new session advanced before reset");
    session.reset(plan);
    require(session.map().paths().size() == 2 && session.gameTick() == 0,
            "reset did not initialize map and logical tick");
    const auto bottom = session.evaluateBuild(QPointF(90, 799), TowerType::Normal);
    require(bottom.valid() && bottom.center == QPointF(90, 750),
            "bottom placement no longer clamps to the final visible row");
    require(session.evaluateBuild(QPointF(90, 230), TowerType::Normal).problem
                == GameSession::BuildProblem::Road,
            "road placement was accepted");
    require(session.evaluateBuild(QPointF(90, 59), TowerType::Normal).problem
                == GameSession::BuildProblem::OutsideMap,
            "HUD placement was accepted");
    require(!session.evaluateBuild(QPointF(std::numeric_limits<double>::infinity(), 100),
                                   TowerType::Normal).valid(),
            "non-finite placement was accepted");
    require(session.build(QPointF(90, 799), TowerType::Normal).success
                && session.gold() == 100 && session.towerCount() == 1,
            "build did not create one tower and deduct its cost");
    require(session.evaluateBuild(QPointF(90, 750), TowerType::Normal).problem
                == GameSession::BuildProblem::Occupied,
            "occupied placement was accepted");
    require(!session.build(QPointF(90, 230), TowerType::Heavy).success
                && session.gold() == 100 && session.towerCount() == 1,
            "failed build changed gold or towers");
    require(session.selectTowerAt(QPointF(90, 750)) && session.selectedTower() != nullptr,
            "tower selection failed");
    require(session.upgradeSelectedTower().success && session.gold() == 50
                && session.selectedTower()->level() == 2
                && session.selectedTower()->bodyDiameter() == 38,
            "upgrade did not preserve cost and body geometry");
    const auto insufficient = session.upgradeSelectedTower();
    require(!insufficient.success && insufficient.message == QStringLiteral("金币不足，无法升级。")
                && session.gold() == 50 && session.selectedTower()->level() == 2,
            "failed upgrade changed state or feedback");
    require(session.sellSelectedTower().success && session.gold() == 100
                && session.towerCount() == 0 && session.selectedTower() == nullptr,
            "sale did not refund half invested cost and clear selection");
    require(!session.sellSelectedTower().success && !session.upgradeSelectedTower().success
                && session.gold() == 100,
            "unselected actions changed gold");

    GameSessionProductTestAccess::setGold(session, 1000);
    require(session.build(QPointF(90, 750), TowerType::Fast).success
                && session.selectTowerAt(QPointF(90, 750)),
            "max-level fixture build failed");
    require(session.upgradeSelectedTower().success && session.upgradeSelectedTower().success,
            "two valid upgrades did not reach level three");
    const int before = session.gold();
    require(!session.upgradeSelectedTower().success && session.gold() == before
                && session.selectedTower()->level() == 3,
            "max-level upgrade changed state");
    session.clearSelectedTower();
    require(session.selectedTower() == nullptr, "clear selection retained a pointer");
}

void checkTowerTargetAndPhase()
{
    // 先给空列表耗尽一次冷却，再放首个与最近目标，分别约束相位和输入顺序选敌。
    const QVector<QPointF> nearPath{{10, 0}, {10000, 0}};
    const QVector<QPointF> nearerPath{{5, 0}, {10000, 0}};
    Enemy first("bug", nearPath), nearest("bug", nearerPath), dead("bug", nearerPath);
    Enemy reached("bug", QVector<QPointF>{{0, 0}});
    dead.takeDamage(1000);
    reached.update();
    QVector<Enemy*> targets{&dead, &reached, &first, &nearest};
    Tower normal(QPointF(0, 0), TowerType::Normal);
    for (int tick = 0; tick < 25; ++tick)
        require(normal.updateAttack({}) == nullptr, "empty tower produced a target");
    for (int tick = 0; tick < 24; ++tick)
        require(normal.updateAttack(targets) == nullptr, "empty interval did not reset attack phase");
    require(normal.updateAttack(targets) == &first,
            "tower did not choose first living unfinished target rather than nearest");
    Tower fast(QPointF(0, 0), TowerType::Fast);
    for (int interval : {10, 7, 7}) {
        for (int tick = 0; tick < interval - 1; ++tick)
            require(fast.updateAttack(targets) == nullptr, "Fast upgrade interval became shorter");
        require(fast.updateAttack(targets) == &first, "Fast 10/7/7 interval changed");
        fast.upgrade();
    }
}

void checkAttackAndLifetime(const AttackPlan& plan)
{
    GameSession session;
    session.reset(plan);
    Enemy* target = GameSessionProductTestAccess::spawn(session, QStringLiteral("virus"));
    target->takeDamage(128); // 护甲 8：130 - (128 - 8) = 10。
    require(target->hp() == 10, "combat fixture did not use product armor");
    require(session.build(QPointF(150, 150), TowerType::Normal).success,
            "combat tower build failed");
    for (int tick = 0; tick < 24; ++tick) session.step();
    require(session.bulletCount() == 0 && target->hp() == 10,
            "tower fired before its 25-tick interval");
    session.step();
    require(session.bulletCount() == 1
                && session.bulletAt(0).target() == target
                && session.bulletAt(0).position() != session.towerAt(0).position(),
            "new bullet did not target first enemy and move in its creation tick");
    for (int tick = 0; tick < 40 && GameSessionProductTestAccess::containsEnemy(session, target);
         ++tick) {
        session.step();
    }
    require(!GameSessionProductTestAccess::containsEnemy(session, target)
                && session.gold() == 125,
            "real bullet did not apply armor damage and award one Virus reward");
    for (int tick = 0; tick < 3; ++tick) session.step();
    require(session.gold() == 125, "dead enemy paid its reward more than once");

    session.reset(plan);
    target = GameSessionProductTestAccess::spawn(session, QStringLiteral("bug"));
    // 第一弹本 tick 已移动但仍存活；后更新的第二弹击杀目标。
    // 这会实际走到敌人删除前的关联子弹清理，而非只测失效弹的常规删除。
    GameSessionProductTestAccess::addBullet(session, QPointF(900, 750), target, 1);
    GameSessionProductTestAccess::addBullet(session, target->position(), target, 100);
    session.step();
    require(session.enemyCount() == 0 && session.bulletCount() == 0 && session.gold() == 160,
            "enemy cleanup retained an earlier bullet targeting the deleted enemy");
}

void checkBossLifecycle(const AttackPlan& plan)
{
    // 击杀与漏怪必须同样撤销光环，并检查离场之后新生成敌人不继承失效全局标志。
    for (bool exits : {false, true}) {
        GameSession session;
        session.reset(plan);
        Enemy* bug = GameSessionProductTestAccess::spawn(session, QStringLiteral("bug"), 3);
        require(bug->maxHp() == 52, "late-game fixture base HP is wrong");
        Enemy* boss = GameSessionProductTestAccess::spawn(session, QStringLiteral("boss"), 5);
        require(bug->maxHp() == 78 && boss->maxHp() == 1072,
                "Boss spawn did not apply aura after late enhancement and append");
        Enemy* virus = GameSessionProductTestAccess::spawn(session, QStringLiteral("virus"), 5);
        require(virus->maxHp() == 253, "new enemy did not receive active Boss aura");
        if (exits) moveToEnd(*boss);
        else boss->takeDamage(100000);
        const auto result = session.step();
        require(result.advanced && session.enemyCount() == 2
                    && bug->maxHp() == 52 && bug->hp() == 52
                    && virus->maxHp() == 169 && virus->hp() == 169,
                "Boss departure did not restore existing living enemies");
        require(session.life() == (exits ? 5 : 10)
                    && session.gold() == (exits ? 150 : 250),
                "Boss reward and leak branch differ from prior behavior");
        Enemy* after = GameSessionProductTestAccess::spawn(session, QStringLiteral("bug"), 5);
        require(after->maxHp() == 52,
                "enemy spawned after Boss departure retained a stale aura");
    }
}

void checkPauseResetAndResults(const AttackPlan& plan)
{
    // 这些检查只覆盖规则状态；页面计时器与隐藏行为由 gui_product 使用真实 Qt 事件验证。
    GameSession session;
    session.reset(plan);
    require(session.build(QPointF(90, 750), TowerType::Normal).success
                && session.selectTowerAt(QPointF(90, 750)),
            "pause fixture build failed");
    session.step();
    const qint64 pausedTick = session.gameTick();
    const Tower* selected = session.selectedTower();
    require(session.setPaused(true) && session.isPaused(), "pause state transition failed");
    require(!session.step().advanced && session.gameTick() == pausedTick
                && !session.build(QPointF(210, 750), TowerType::Normal).success
                && !session.upgradeSelectedTower().success
                && !session.sellSelectedTower().success
                && !session.selectTowerAt(QPointF(90, 750))
                && session.gold() == 100 && session.selectedTower() == selected,
            "paused session advanced or accepted a gameplay action");
    require(session.setPaused(false) && session.step().advanced,
            "resume did not allow the next logical tick");
    session.reset(plan, false);
    require(session.state() == GameSession::State::Stopped && session.gameTick() == 0
                && session.enemyCount() == 0 && session.towerCount() == 0
                && session.bulletCount() == 0 && session.selectedTower() == nullptr
                && session.gold() == 150 && session.life() == 10
                && !session.step().advanced && !session.setPaused(false),
            "stopped reset retained state or restarted itself");

    session.reset(plan);
    GameSessionProductTestAccess::setLife(session, 1);
    Enemy* boss = GameSessionProductTestAccess::spawn(session, QStringLiteral("boss"), 5);
    moveToEnd(*boss);
    boss->takeDamage(100000); // 同时到终点和死亡，仍按漏怪优先。
    session.step();
    require(session.state() == GameSession::State::Lost && session.life() == -4
                && session.gold() == 150 && !session.step().advanced,
            "loss or endpoint-before-death priority changed");

    session.reset(plan);
    GameSessionProductTestAccess::setLife(session, 10000);
    int finalWaveSignals = 0;
    for (int tick = 0; tick < 20000 && !session.isFinished(); ++tick) {
        const auto frame = session.step();
        if (frame.finalWaveStarted) ++finalWaveSignals;
    }
    require(session.isWon() && session.life() > 0 && session.enemyCount() == 0
                && session.scheduler().spawnedCount() == 90
                && !session.scheduler().hasPendingGeneration() && finalWaveSignals == 1,
            "complete real session did not win after all 90 enemies left");
    const qint64 finishedTick = session.gameTick();
    require(!session.step().advanced && session.gameTick() == finishedTick,
            "finished session continued advancing");
    session.clear();
    require(session.state() == GameSession::State::Stopped && session.gameTick() == 0
                && session.scheduler().totalEnemies() == 0,
            "clear retained the old attack plan or result");

    session.reset(plan);
    GameSessionProductTestAccess::setLife(session, 10000);
    while (session.scheduler().hasPendingGeneration()) session.step();
    GameSessionProductTestAccess::killAll(session);
    GameSessionProductTestAccess::setLife(session, 0);
    session.step();
    require(session.state() == GameSession::State::Lost && session.enemyCount() == 0
                && session.scheduler().spawnedCount() == 90,
            "loss must take precedence when generation and cleanup also satisfy victory");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const auto loaded = AttackPlanLoader::load(QStringLiteral(":/config/attack_waves.json"), QString());
    require(loaded.plan.has_value(), "could not load built-in attack plan");
    checkConstructionAndActions(*loaded.plan);
    checkTowerTargetAndPhase();
    checkAttackAndLifetime(*loaded.plan);
    checkBossLifecycle(*loaded.plan);
    checkPauseResetAndResults(*loaded.plan);
    std::cout << "PASS: GameSession product placement, combat, lifecycle, pause, reset, and results\n";
}
