#include "messages/lazyloadedimage.h"

#include "asyncexec.h"
#include "emotemanager.h"
#include "ircmanager.h"
#include "windowmanager.h"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <functional>

namespace chatterino {
namespace messages {

LazyLoadedImage::LazyLoadedImage(const QString &url, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : currentPixmap(NULL)
    , allFrames()
    , currentFrame(0)
    , currentFrameOffset(0)
    , url(url)
    , name(name)
    , tooltip(tooltip)
    , animated(false)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(false)
{
}

LazyLoadedImage::LazyLoadedImage(QPixmap *image, qreal scale, const QString &name,
                                 const QString &tooltip, const QMargins &margin, bool isHat)
    : currentPixmap(image)
    , allFrames()
    , currentFrame(0)
    , currentFrameOffset(0)
    , url()
    , name(name)
    , tooltip(tooltip)
    , animated(false)
    , margin(margin)
    , ishat(isHat)
    , scale(scale)
    , isLoading(true)
{
}

void LazyLoadedImage::loadImage()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    QUrl url(this->url);
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        QByteArray array = reply->readAll();
        QBuffer buffer(&array);
        buffer.open(QIODevice::ReadOnly);

        QImage image;
        QImageReader reader(&buffer);

        bool first = true;

        for (int index = 0; index < reader.imageCount(); ++index) {
            if (reader.read(&image)) {
                auto pixmap = new QPixmap(QPixmap::fromImage(image));

                if (first) {
                    first = false;
                    this->currentPixmap = pixmap;
                }

                FrameData data;
                data.duration = std::max(20, reader.nextImageDelay());
                data.image = pixmap;

                allFrames.push_back(data);
            }
        }

        if (allFrames.size() > 1) {
            this->animated = true;

            EmoteManager::getInstance().getGifUpdateSignal().connect([this] { gifUpdateTimout(); });
        }

        EmoteManager::getInstance().incGeneration();
        WindowManager::getInstance().layoutVisibleChatWidgets();

        reply->deleteLater();
        manager->deleteLater();
    });
}

void LazyLoadedImage::gifUpdateTimout()
{
    this->currentFrameOffset += GIF_FRAME_LENGTH;

    while (true) {
        if (this->currentFrameOffset > this->allFrames.at(this->currentFrame).duration) {
            this->currentFrameOffset -= this->allFrames.at(this->currentFrame).duration;
            this->currentFrame = (this->currentFrame + 1) % this->allFrames.size();
        } else {
            break;
        }
    }

    this->currentPixmap = this->allFrames[this->currentFrame].image;
}
}  // namespace messages
}  // namespace chatterino