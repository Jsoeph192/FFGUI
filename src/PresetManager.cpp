#include "PresetManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>

PresetManager::PresetManager(QObject *parent)
    : QObject(parent)
{
    QDir dir(getPresetDirectory());
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QStringList PresetManager::getAvailablePresets() const
{
    QDir dir(getPresetDirectory());
    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files);
    
    QStringList presets;
    for (const QString& file : files) {
        presets.append(QFileInfo(file).baseName());
    }
    
    return presets;
}

QVariantMap PresetManager::loadPreset(const QString& name) const
{
    QString filePath = getPresetFilePath(name);
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        return QVariantMap();
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QVariantMap map;
        
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            map[it.key()] = it.value().toVariant();
        }
        
        return map;
    }
    
    return QVariantMap();
}

bool PresetManager::savePreset(const QString& name, const QVariantMap& preset)
{
    QString filePath = getPresetFilePath(name);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonObject obj;
    for (auto it = preset.begin(); it != preset.end(); ++it) {
        obj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    
    QJsonDocument doc(obj);
    file.write(doc.toJson());
    file.close();
    
    return true;
}

bool PresetManager::deletePreset(const QString& name)
{
    QString filePath = getPresetFilePath(name);
    return QFile::remove(filePath);
}

QString PresetManager::getPresetDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/presets";
}

QString PresetManager::getPresetFilePath(const QString& name) const
{
    return getPresetDirectory() + "/" + name + ".json";
}
