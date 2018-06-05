#include "singletons/loggingmanager.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/settingsmanager.hpp"

#include <QDir>
#include <QStandardPaths>

#include <unordered_map>

namespace chatterino {
namespace singletons {

void LoggingManager::initialize()
{
    this->pathManager = getApp()->paths;
}

void LoggingManager::addMessage(const QString &channelName, messages::MessagePtr message)
{
    auto app = getApp();

    if (!app->settings->enableLogging) {
        return;
    }

    auto it = this->loggingChannels.find(channelName);
    if (it == this->loggingChannels.end()) {
        auto channel = new LoggingChannel(channelName, this->getDirectoryForChannel(channelName));
        channel->addMessage(message);
        this->loggingChannels.emplace(channelName,
                                      std::unique_ptr<LoggingChannel>(std::move(channel)));
    } else {
        it->second->addMessage(message);
    }
}

QString LoggingManager::getDirectoryForChannel(const QString &channelName)
{
    //    auto customPath = getApp()->settings->logPath;
    auto customPath = QString("");

    if (channelName.startsWith("/whispers")) {
        if (customPath != "")
            return customPath + "/Whispers";
        return this->pathManager->whispersLogsFolderPath;
    } else if (channelName.startsWith("/mentions")) {
        if (customPath != "")
            return customPath + "/Mentions";
        return this->pathManager->mentionsLogsFolderPath;
    } else {
        if (customPath != "") {
            QString logPath(customPath + QDir::separator() + channelName);

            if (!this->pathManager->createFolder(logPath)) {
                debug::Log("Error creating channel logs folder for channel {}", channelName);
            }

            return logPath;
        }
        QString logPath(this->pathManager->channelsLogsFolderPath + QDir::separator() +
                        channelName);

        if (!this->pathManager->createFolder(logPath)) {
            debug::Log("Error creating channel logs folder for channel {}", channelName);
        }

        return logPath;
    }
}

void LoggingManager::refreshLoggingPath()
{
    // if (app->settings->logPath == "") {

    //}
}

}  // namespace singletons
}  // namespace chatterino
