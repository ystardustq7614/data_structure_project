#ifndef GAMERENDERER_H
#define GAMERENDERER_H

#include "core/gamesession.h"
#include "gameuistate.h"
#include <QHash>
#include <QPixmap>
#include <QRect>
#include <array>
class QPainter;

// 只读展示层：统一实体、地图与覆盖层绘制，并提供输入复用的命中布局。
// 图片缓存只属于展示层；绘制不能推进、建造或修改任何战局对象。
class GameRenderer
{
public:
    using Control = GameControl;
    GameRenderer();
    void paint(QPainter& painter, const GameSession& session, const GameUiState& ui) const;
    void drawEnemy(QPainter& painter, const Enemy& enemy) const;
    void drawTower(QPainter& painter, const Tower& tower, bool selected) const;
    void drawBullet(QPainter& painter, const Bullet& bullet) const;
    QRect controlRect(Control control, const GameSession& session) const;
    QRect previewPanelRect() const;
    Control controlAt(const QPoint& pos, const GameSession& session) const;
    bool blocksMapInput(const QPointF& pos, const GameSession& session,
                        const GameUiState& ui) const;
    GameSession::BuildEvaluation evaluateBuild(const QPointF& pos,
                                              const GameSession& session,
                                              const GameUiState& ui) const;
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
    QHash<QString, QPixmap> m_enemyIcons;
    std::array<QPixmap, 3> m_towerPortraits;
};

#endif
