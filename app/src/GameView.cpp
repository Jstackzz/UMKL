#include "GameView.h"

#include "GameSession.h"

#include <QQuickWindow>
#include <QSGSimpleTextureNode>

GameView::GameView(QQuickItem* parent) : QQuickItem(parent) { setFlag(ItemHasContents, true); }

void GameView::setSession(GameSession* s)
{
    if (session_ == s)
        return;
    if (session_)
        disconnect(session_, nullptr, this, nullptr);
    session_ = s;
    if (session_)
        connect(session_, &GameSession::frameReady, this, &QQuickItem::update);
    emit sessionChanged();
    update();
}

void GameView::setFiltered(bool f)
{
    if (filtered_ == f)
        return;
    filtered_ = f;
    dirty_ = true;
    emit filteredChanged();
    update();
}

void GameView::clear()
{
    img_ = QImage();
    if (hasFrame_) {
        hasFrame_ = false;
        emit hasFrameChanged();
    }
    dirty_ = true;
    update();
}

QSGNode* GameView::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    auto* node = static_cast<QSGSimpleTextureNode*>(old);
    bool fresh = false;
    if (session_ && session_->frames()->take(img_, serial_))
        fresh = true;
    if (img_.isNull()) {
        delete node;
        return nullptr;
    }
    if (!node) {
        node = new QSGSimpleTextureNode;
        node->setOwnsTexture(true);
        fresh = true;
    }
    if (fresh || dirty_ || !node->texture()) {
        QSGTexture* t = window()->createTextureFromImage(img_, QQuickWindow::TextureIsOpaque);
        node->setTexture(t);
        dirty_ = false;
        if (!hasFrame_) {
            hasFrame_ = true;
            QMetaObject::invokeMethod(this, [this] { emit hasFrameChanged(); }, Qt::QueuedConnection);
        }
    }
    node->setFiltering(filtered_ ? QSGTexture::Linear : QSGTexture::Nearest);
    node->setRect(boundingRect());
    return node;
}
