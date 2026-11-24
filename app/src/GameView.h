#pragma once
// The running game's picture as a QML item: the newest frame from GameSession, uploaded as a texture and drawn over
// the item's whole rectangle (QML sizes the item to the shape it wants: 4:3, stretched or whole pixels).
#include <QImage>
#include <QQuickItem>

class GameSession;

class GameView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(GameSession* session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(bool filtered READ filtered WRITE setFiltered NOTIFY filteredChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)
    Q_PROPERTY(int sourceWidth READ sourceWidth NOTIFY hasFrameChanged)
    Q_PROPERTY(int sourceHeight READ sourceHeight NOTIFY hasFrameChanged)

public:
    explicit GameView(QQuickItem* parent = nullptr);

    GameSession* session() const { return session_; }
    void setSession(GameSession* s);
    bool filtered() const { return filtered_; }
    void setFiltered(bool f);
    bool hasFrame() const { return hasFrame_; }
    int sourceWidth() const { return img_.isNull() ? 400 : img_.width(); }
    int sourceHeight() const { return img_.isNull() ? 254 : img_.height(); }
    Q_INVOKABLE void clear();

signals:
    void sessionChanged();
    void filteredChanged();
    void hasFrameChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override;

private:
    GameSession* session_ = nullptr;
    QImage img_;
    quint64 serial_ = 0;
    bool filtered_ = false;
    bool hasFrame_ = false;
    bool dirty_ = false;
};
