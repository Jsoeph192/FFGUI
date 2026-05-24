#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    qDebug() << "=== APPLICATION STARTUP DIAGNOSTICS ===";
    qDebug() << "Current directory:" << QDir::currentPath();
    
    QStringList importantFiles = {
        "libvlc.dll",
        "libvlccore.dll", 
        "plugins/",
        "Qt6Core.dll"
    };
    
    for (const QString& file : importantFiles) {
        QFileInfo fileInfo(file);
        if (fileInfo.exists()) {
            if (fileInfo.isDir()) {
                qDebug() << file << ": DIRECTORY EXISTS (" << fileInfo.absoluteFilePath() << ")";
            } else {
                qDebug() << file << ": FILE EXISTS (" << fileInfo.absoluteFilePath() << ")";
            }
        } else {
            qDebug() << file << ": NOT FOUND";
        }
    }
    
    QDir pluginsDir("plugins");
    if (pluginsDir.exists()) {
        qDebug() << "Plugins directory contains" << pluginsDir.entryList(QStringList() << "*.dll", QDir::Files).count() << "plugin files";
    }
    
    qDebug() << "=== END DIAGNOSTICS ===";
    
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
