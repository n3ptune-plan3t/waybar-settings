// waybar-settings: a Qt6 Widgets settings manager for Waybar.
//
// It edits ~/.config/waybar/config.jsonc (bar options, module layout and every
// per-module option) plus a generated block in style.css. Comments and unknown
// keys in the config survive a round trip; so does anything in style.css that
// lies outside the waybar-settings markers.
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("waybar-settings"));
  QApplication::setApplicationVersion(QStringLiteral("2.0"));
  ws::MainWindow window;
  window.show();
  return app.exec();
}
