/// @file plugins/bin/snail/helpers.h

#ifndef PLUGIN_SNAIL_HELPERS_H
#define SNAIL_LIBS_NVIMCORE_HELPERS_H

#include "plugins/bin/snail/shellcontents.h"

bool saveShellContents(const ShellContents &s, const QString &filename);
bool isBadMonospace(const QFont &f);

#endif // PLUGIN_SNAIL_HELPERS_H
