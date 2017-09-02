// Markus Kuhn -- 2001-01-12 -- public domain
// Adaptions for KDE by Waldo Bastian <bastian@kde.org>

/// @headerfile ""

#ifndef SNAIL_LIBS_NVIMCORE_KONSOLE_WCWIDTH_H
#define SNAIL_LIBS_NVIMCORE_KONSOLE_WCWIDTH_H

#include <QtCore/QString>

int konsole_wcwidth(quint16 oucs);
int string_width(const QString &text);

#endif // SNAIL_LIBS_NVIMCORE_KONSOLE_WCWIDTH_H
