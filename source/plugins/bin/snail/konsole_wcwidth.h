/// @file plugins/bin/snail/konsole_wcwidth.h

#ifndef PLUGIN_SNAIL_KONSOLE_WCWIDTH_H
#define PLUGIN_SNAIL_KONSOLE_WCWIDTH_H

#include <QtCore/QString>

int konsole_wcwidth(quint16 oucs);
int string_width(const QString &text);

#endif // PLUGIN_SNAIL_KONSOLE_WCWIDTH_H
