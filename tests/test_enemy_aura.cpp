#include <iostream>
#include <cassert>
#include <vector>
#include <string>

// Standalone C++ verification of Enemy aura & buff mechanics extracted from enemy.cpp
class TestEnemy {
public:
    std::string m_type;
    int m_hp = 30;
    int m_maxHp = 30;
    int m_baseMaxHp = 30;
    int m_armor = 0;
    int m_baseArmor = 0;
    bool m_lateGameEnhanced = false;
    bool m_bossAuraApplied = false;
    bool m_hasMutated = false;

    TestEnemy(const std::string& type) : m_type(type) {
        if (type == "bug") {
            m_hp = 40; m_maxHp = 40; m_baseMaxHp = 40;
            m_armor = 0; m_baseArmor = 0;
        } else if (type == "ddl") {
            m_hp = 28; m_maxHp = 28; m_baseMaxHp = 28;
            m_armor = 0; m_baseArmor = 0;
        } else if (type == "virus") {
            m_hp = 130; m_maxHp = 130; m_baseMaxHp = 130;
            m_armor = 8; m_baseArmor = 8;
        } else if (type == "boss") {
            m_hp = 550; m_maxHp = 550; m_baseMaxHp = 550;
            m_armor = 18; m_baseArmor = 18;
        } else {
            m_hp = 30; m_maxHp = 30; m_baseMaxHp = 30;
            m_armor = 0; m_baseArmor = 0;
        }
    }

    bool isDead() const { return m_hp <= 0; }
    int armor() const { return m_armor; }

    int calculateMaxHp() const {
        int hp = m_baseMaxHp;
        if (m_lateGameEnhanced) {
            hp = static_cast<int>(hp * 1.3);
        }
        if (m_bossAuraApplied) {
            hp = static_cast<int>(hp * 1.5);
        }
        return hp;
    }

    int calculateArmor() const {
        int arm = m_baseArmor;
        if (m_lateGameEnhanced) {
            arm += 3;
        }
        return arm;
    }

    void takeDamage(int damage) {
        int realDamage = damage - m_armor;
        if (realDamage < 1) {
            realDamage = 1;
        }
        m_hp -= realDamage;
    }

    void applyLateGameEnhance() {
        if (m_lateGameEnhanced) return;
        m_lateGameEnhanced = true;
        int oldMaxHp = m_maxHp;
        m_maxHp = calculateMaxHp();
        m_armor = calculateArmor();
        if (oldMaxHp > 0) {
            m_hp = static_cast<int>(m_hp * 1.3);
            if (m_hp > m_maxHp) m_hp = m_maxHp;
        }
    }

    void applyBossAura() {
        if (m_bossAuraApplied) return;
        m_bossAuraApplied = true;
        int oldMaxHp = m_maxHp;
        m_maxHp = calculateMaxHp();
        if (oldMaxHp > 0 && m_hp > 0) {
            m_hp = static_cast<int>((static_cast<long long>(m_hp) * m_maxHp + oldMaxHp / 2) / oldMaxHp);
            if (m_hp > m_maxHp) m_hp = m_maxHp;
        }
    }

    void removeBossAura() {
        if (!m_bossAuraApplied) return;
        m_bossAuraApplied = false;
        int oldMaxHp = m_maxHp;
        m_maxHp = calculateMaxHp();
        if (isDead() || m_hp <= 0) {
            m_hp = 0;
            return;
        }
        if (oldMaxHp > 0) {
            m_hp = static_cast<int>((static_cast<long long>(m_hp) * m_maxHp + oldMaxHp / 2) / oldMaxHp);
            if (m_hp > m_maxHp) m_hp = m_maxHp;
            if (m_hp <= 0) m_hp = 1;
        }
    }

    void mutateVirus() {
        m_hasMutated = true;
        m_type = "mutant";
        m_baseMaxHp = 160;
        m_baseArmor = 12;
        m_maxHp = calculateMaxHp();
        m_armor = calculateArmor();
        m_hp += 120;
        if (m_hp > m_maxHp) m_hp = m_maxHp;
    }
};

int main() {
    std::cout << "[Test 1] Codex regression case: Late Virus with 3 HP aura round-trip" << std::endl;
    {
        TestEnemy enemy("virus");
        enemy.applyLateGameEnhance();
        assert(enemy.m_maxHp == 169 && enemy.armor() == 11);
        enemy.takeDamage(enemy.armor() + 166);
        assert(enemy.m_hp == 3);

        enemy.applyBossAura();
        assert(enemy.m_maxHp == 253);
        assert(enemy.m_hp == 4); // scaled up: (3 * 253 + 84) / 169 = 4

        enemy.removeBossAura();
        assert(enemy.m_maxHp == 169);
        assert(enemy.m_hp == 3); // zero-damage round-trip successfully preserved 3 HP!

        enemy.takeDamage(enemy.armor() + 2);
        assert(enemy.m_hp == 1);
        assert(!enemy.isDead());
        std::cout << "  -> Passed: HP correctly preserved, enemy survived with 1 HP." << std::endl;
    }

    std::cout << "[Test 2] Exhaustive zero-damage round-trip for all enemy types and states" << std::endl;
    std::vector<std::pair<std::string, bool>> configs = {
        {"bug", false}, {"bug", true},
        {"ddl", false}, {"ddl", true},
        {"virus", false}, {"virus", true},
        {"boss", false}, {"boss", true}
    };

    int totalTested = 0;
    for (const auto& cfg : configs) {
        TestEnemy proto(cfg.first);
        if (cfg.second) proto.applyLateGameEnhance();
        int maxHp = proto.m_maxHp;

        for (int initialHp = 1; initialHp <= maxHp; ++initialHp) {
            TestEnemy e(cfg.first);
            if (cfg.second) e.applyLateGameEnhance();
            e.m_hp = initialHp;

            e.applyBossAura();
            e.removeBossAura();

            assert(e.m_hp == initialHp);
            assert(e.m_maxHp == maxHp);
            totalTested++;
        }
    }
    std::cout << "  -> Passed: " << totalTested << " HP states tested without a single point of loss." << std::endl;

    std::cout << "[Test 3] Mutant under aura round-trip" << std::endl;
    {
        TestEnemy e("virus");
        e.applyLateGameEnhance();
        e.applyBossAura();
        e.mutateVirus();
        assert(e.m_maxHp == 312);
        assert(e.m_armor == 15);

        // Exhaustive test for mutant under aura
        for (int mHp = 1; mHp <= 208; ++mHp) {
            TestEnemy m("virus");
            m.applyLateGameEnhance();
            m.mutateVirus();
            assert(m.m_maxHp == 208);
            m.m_hp = mHp;
            m.applyBossAura();
            assert(m.m_maxHp == 312);
            m.removeBossAura();
            assert(m.m_maxHp == 208);
            assert(m.m_hp == mHp);
        }
        std::cout << "  -> Passed: All 208 mutant HP values preserved under aura round-trip." << std::endl;
    }

    std::cout << "[Test 4] Dead enemy protection and 1 HP protection" << std::endl;
    {
        TestEnemy deadEnemy("virus");
        deadEnemy.applyLateGameEnhance();
        deadEnemy.m_hp = 0;
        deadEnemy.applyBossAura();
        assert(deadEnemy.m_hp == 0);
        deadEnemy.removeBossAura();
        assert(deadEnemy.m_hp == 0);
        assert(deadEnemy.isDead());

        TestEnemy lowEnemy("bug");
        lowEnemy.m_hp = 1;
        lowEnemy.applyBossAura();
        assert(lowEnemy.m_hp >= 1);
        lowEnemy.removeBossAura();
        assert(lowEnemy.m_hp == 1);
        assert(!lowEnemy.isDead());
        std::cout << "  -> Passed: Dead enemy stays dead; low HP enemy never dies from removal." << std::endl;
    }

    std::cout << "ALL C++ TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
