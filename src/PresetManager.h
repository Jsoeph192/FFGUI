#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>

class PresetManager : public QObject
{
    Q_OBJECT

public:
    explicit PresetManager(QObject *parent = nullptr);
    
    QStringList getAvailablePresets() const;
    QVariantMap loadPreset(const QString& name) const;
    bool savePreset(const QString& name, const QVariantMap& preset);
    bool deletePreset(const QString& name);

private:
    QString getPresetDirectory() const;
    QString getPresetFilePath(const QString& name) const;
};

#endif // PRESETMANAGER_H
