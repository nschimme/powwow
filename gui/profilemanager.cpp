#include "profilemanager.h"

ProfileManager::ProfileManager()
{
}

QStringList ProfileManager::sessionNames() const
{
    QSettings settings("powwow", "powwow-gui");
    QStringList names;
    const auto childGroups = settings.childGroups();
    for (const QString &group : childGroups) {
        if (group.startsWith("session_")) {
            names.append(group.mid(8));
        }
    }
    return names;
}

SessionProfile ProfileManager::loadSession(const QString &name) const
{
    QSettings settings("powwow", "powwow-gui");
    SessionProfile profile;
    profile.name = name;
    QString groupKey = QString("session_%1").arg(name);

    if (settings.childGroups().contains(groupKey)) {
        settings.beginGroup(groupKey);
        profile.host = settings.value("host", "").toString();
        profile.port = settings.value("port", 23).toInt();
        profile.powFilePath = settings.value("powFilePath", "").toString();
        profile.editor = settings.value("editor", "").toString();
        profile.pager = settings.value("pager", "").toString();
        profile.useTls = settings.value("useTls", false).toBool();
        profile.useUtf8 = settings.value("useUtf8", true).toBool();
        settings.endGroup();
    }
    return profile;
}

void ProfileManager::saveSession(const SessionProfile &profile)
{
    if (profile.name.trimmed().isEmpty()) {
        return;
    }
    QSettings settings("powwow", "powwow-gui");
    QString groupKey = QString("session_%1").arg(profile.name.trimmed());
    settings.beginGroup(groupKey);
    settings.setValue("host", profile.host);
    settings.setValue("port", profile.port);
    settings.setValue("powFilePath", profile.powFilePath);
    settings.setValue("editor", profile.editor);
    settings.setValue("pager", profile.pager);
    settings.setValue("useTls", profile.useTls);
    settings.setValue("useUtf8", profile.useUtf8);
    settings.endGroup();
    settings.sync();
}

void ProfileManager::deleteSession(const QString &name)
{
    QSettings settings("powwow", "powwow-gui");
    QString groupKey = QString("session_%1").arg(name);
    settings.remove(groupKey);
    settings.sync();
}
