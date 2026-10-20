/** @file
 * @brief 战局的只读绘制与共享命中布局；不负责输入事件、模拟推进或实体所有权。 */
#ifndef GAMERENDERER_H
#define GAMERENDERER_H

#include "core/gamesession.h"
#include "gameuistate.h"
#include <QHash>
#include <QPixmap>
#include <QRect>
#include <array>
class QPainter;

/** 固定逻辑画布的展示器；构造时加载嵌入图像并按本对象生命周期缓存，缺图使用备用绘制。
 * 缓存不热更新；输入必须复用本类布局，不另存一份按钮或遮挡坐标。
 */
class GameRenderer
{
public:
    using Control = GameControl;
    /** 在 GUI 环境建立后加载背景与实体贴图；缺资源不阻止构造，绘制时采用备用图形。 */
    GameRenderer();
    /** 向活动 painter 绘制 session/ui 当前快照；不推进状态，逻辑坐标不随设备像素比改变。
     * 本方法会改变 painter 的画笔/字体等状态，调用者负责其绘制生命周期；无错误返回。
     */
    void paint(QPainter& painter, const GameSession& session, const GameUiState& ui) const;
    /** 向活动 painter 绘制敌人贴图/备用图形及生命条；改变画笔/画刷，不推进 enemy。 */
    void drawEnemy(QPainter& painter, const Enemy& enemy) const;
    /** 绘制 tower 当前等级的塔身，selected 决定是否画射程；内部保存并恢复 painter 状态。 */
    void drawTower(QPainter& painter, const Tower& tower, bool selected) const;
    /** 仅绘制未结束 bullet 的当前位置，不推进追踪或造成伤害；可改变 painter 画笔状态。 */
    void drawBullet(QPainter& painter, const Bullet& bullet) const;
    /** 返回 control 在当前胜负布局中的逻辑矩形；None/未知标识返回空，不判断按钮是否可用。 */
    QRect controlRect(Control control, const GameSession& session) const;
    /** 返回固定预览面板范围；是否显示/遮挡仍由 ui.previewVisible 决定。 */
    QRect previewPanelRect() const;
    /** 按 session 状态返回 pos 命中的可用控件；未命中返回 None，不执行按钮动作。 */
    Control controlAt(const QPoint& pos, const GameSession& session) const;
    /** 判断 pos 是否非有限或被 HUD、当前可用按钮、已展开面板拦截；运行态仍由战局校验。 */
    bool blocksMapInput(const QPointF& pos, const GameSession& session,
                        const GameUiState& ui) const;
    /** 界面遮挡拦截时返回 OutsideMap；其余按 ui.selectedTowerType 透传战局判定结果。 */
    GameSession::BuildEvaluation evaluateBuild(const QPointF& pos,
                                              const GameSession& session,
                                              const GameUiState& ui) const;
    /** 返回 type 的界面名称；未知枚举使用普通塔名称，无实体或资源加载副作用。 */
    static QString towerName(TowerType type);

private:
    static constexpr int width() { return GameMap::Width; }
    static constexpr int height() { return GameMap::Height; }
    static QRect rect() { return QRect(0, 0, width(), height()); }
    QRect resultRetryRect(const GameSession& session) const;
    QRect resultMenuRect(const GameSession& session) const;
    bool hasTowerAt(const QPointF& pos, const GameSession& session) const;
    void drawBackground(QPainter&, const GameSession&, const GameUiState&) const;
    void drawGrid(QPainter&, const GameSession&, const GameUiState&) const;
    void drawPath(QPainter&, const GameSession&, const GameUiState&) const;
    void drawHud(QPainter&, const GameSession&, const GameUiState&) const;
    void drawMapMarkers(QPainter&, const GameSession&, const GameUiState&) const;
    void drawStatusMessage(QPainter&, const GameSession&, const GameUiState&) const;
    void drawGameResultScreen(QPainter&, const GameSession&, const GameUiState&) const;
    void drawBuildPreview(QPainter&, const GameSession&, const GameUiState&) const;
    void drawEventPreview(QPainter&, const GameSession&, const GameUiState&) const;
    void drawPauseOverlay(QPainter&, const GameSession&, const GameUiState&) const;
    const QPixmap& towerPortrait(TowerType type) const;
    void drawTowerVisual(QPainter& painter, const QPointF& pos, TowerType type,
                         int diameter, int level) const;
    QPixmap m_gameMapBackground;
    QPixmap m_winImage;
    QPixmap m_loseImage;
    // 键为核心类型字符串，空 pixmap 表示加载失败；构造后不失效、不在 paint 中重新读取。
    QHash<QString, QPixmap> m_enemyIcons;
    std::array<QPixmap, 3> m_towerPortraits;
};

#endif
