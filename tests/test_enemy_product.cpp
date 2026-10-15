#include "../26C-/enemy.h"

#include <QGuiApplication>
#include <QPointF>
#include <QString>
#include <QVector>

#include <cstdlib>
#include <initializer_list>
#include <iostream>

namespace {

const QVector<QPointF> longPath{
    QPointF(0, 0), QPointF(100, 0), QPointF(200, 0), QPointF(300, 0)
};

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void reduceTo(Enemy& enemy, int hp)
{
    require(hp >= 0 && hp <= enemy.maxHp(), "invalid requested HP");
    if (hp < enemy.maxHp()) {
        enemy.takeDamage(enemy.armor() + enemy.maxHp() - hp);
    }
    require(enemy.hp() == hp, "damage setup did not reach requested HP");
}

void checkRoundTrips(const QString& type, bool enhanced, int expectedMaxHp)
{
    for (int hp = 0; hp <= expectedMaxHp; ++hp) {
        Enemy enemy(type, longPath);
        if (enhanced) {
            enemy.applyLateGameEnhance();
        }
        require(enemy.maxHp() == expectedMaxHp, "unexpected initial maximum HP");
        reduceTo(enemy, hp);

        enemy.applyBossAura();
        const int auraMaxHp = enemy.maxHp();
        const int auraHp = enemy.hp();
        require(auraMaxHp == static_cast<int>(expectedMaxHp * 1.5),
                "aura maximum HP is wrong");
        enemy.applyBossAura();
        require(enemy.maxHp() == auraMaxHp && enemy.hp() == auraHp,
                "repeated aura application changed HP");

        enemy.removeBossAura();
        require(enemy.maxHp() == expectedMaxHp && enemy.hp() == hp,
                "damage-free aura round trip changed HP");
        enemy.removeBossAura();
        require(enemy.maxHp() == expectedMaxHp && enemy.hp() == hp,
                "repeated aura removal changed HP");
        require(enemy.isDead() == (hp == 0), "aura changed death state");
    }
}

void checkAuraDamage()
{
    Enemy enemy("virus", longPath);
    enemy.applyLateGameEnhance();
    reduceTo(enemy, 3);
    enemy.applyBossAura();
    require(enemy.maxHp() == 253 && enemy.hp() == 4 && enemy.armor() == 11,
            "late Virus aura properties are wrong");
    enemy.removeBossAura();
    require(enemy.maxHp() == 169 && enemy.hp() == 3,
            "low HP regression: aura round trip lost health");
    enemy.takeDamage(enemy.armor() + 2);
    require(enemy.hp() == 1 && !enemy.isDead(),
            "low HP regression: later damage killed a living enemy");

    for (int hp = 1; hp <= 169; ++hp) {
        for (int damage : {1, 2, 7, 30}) {
            Enemy damaged("virus", longPath);
            damaged.applyLateGameEnhance();
            reduceTo(damaged, hp);
            damaged.applyBossAura();
            damaged.takeDamage(damaged.armor() + damage);
            const bool diedInAura = damaged.isDead();
            damaged.removeBossAura();
            require(damaged.isDead() == diedInAura,
                    "removing aura changed death state after damage");
            require(damaged.hp() >= 0 && damaged.hp() <= damaged.maxHp(),
                    "HP is outside valid range after aura damage");
        }
    }
}

void checkMutation(bool enhanced, bool aura, int expectedMaxHp, int expectedArmor)
{
    const QVector<QPointF> shortPath{QPointF(0, 0), QPointF(100, 0)};
    Enemy enemy("virus", shortPath);
    if (enhanced) {
        enemy.applyLateGameEnhance();
    }
    if (aura) {
        enemy.applyBossAura();
    }
    enemy.update(); // A two-point path triggers Virus mutation before movement.
    require(enemy.damageToLife() == 2 && enemy.reward() == 60,
            "Virus did not mutate");
    require(enemy.maxHp() == expectedMaxHp && enemy.armor() == expectedArmor,
            "mutation overwrote an existing enhancement");
    require(enemy.hp() > 0 && enemy.hp() <= enemy.maxHp(),
            "mutant HP is outside valid range");
    if (aura) {
        enemy.removeBossAura();
        const int unboostedMaxHp = enhanced ? 208 : 160;
        require(enemy.maxHp() == unboostedMaxHp,
                "aura removal did not restore mutant maximum HP");
        require(enemy.hp() > 0 && enemy.hp() <= enemy.maxHp(),
                "aura removal invalidated mutant HP");
    }

    Enemy damaged("virus", shortPath);
    if (enhanced) {
        damaged.applyLateGameEnhance();
    }
    if (aura) {
        damaged.applyBossAura();
    }
    reduceTo(damaged, 10);
    damaged.update();
    require(damaged.hp() == 130,
            "mutation did not preserve the existing +120 current HP rule");
}

} // namespace

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    checkRoundTrips("bug", false, 40);
    checkRoundTrips("bug", true, 52);
    checkRoundTrips("ddl", false, 28);
    checkRoundTrips("ddl", true, 36);
    checkRoundTrips("virus", false, 130);
    checkRoundTrips("virus", true, 169);
    checkRoundTrips("boss", false, 550);
    checkRoundTrips("boss", true, 715);
    checkAuraDamage();
    checkMutation(false, false, 160, 12);
    checkMutation(false, true, 240, 12);
    checkMutation(true, false, 208, 15);
    checkMutation(true, true, 312, 15);

    std::cout << "PASS: Enemy product aura, damage, and mutation regression\n";
    return 0;
}
