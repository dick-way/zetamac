# ----------------------------
# Makefile Options
# ----------------------------

NAME = ZETAMAC
ICON = icon.png
DESCRIPTION = "Arithmetic Speed Drill"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)