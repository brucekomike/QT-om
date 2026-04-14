#include "app/mainwindow.h"
#include <QApplication>
#include <QIcon>

#ifdef USE_KF6
#  include <KAboutData>
#  include <KLocalizedString>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("qt-om");
    app.setApplicationDisplayName("QT-om");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("qt-om");
    app.setOrganizationDomain("qt-om.app");

#ifdef USE_KF6
    KAboutData aboutData(
        "qt-om",
        i18n("QT-om"),
        "0.1.0",
        i18n("Cross-platform Qt DevOps server manager"),
        KAboutLicense::GPL_V3,
        i18n("© 2024 QT-om contributors"));
    KAboutData::setApplicationData(aboutData);
#endif

    MainWindow window;
    window.show();
    return app.exec();
}
