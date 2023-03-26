# SPDX-License-Identifier: GPL-2.0-or-later
set(live_effects_SRC
	effect_ui.cpp
	lpe-fillet-chamfer.cpp
    parameter/parameter_ui.cpp
    parameter/nodesatellitesarray.cpp
    parameter/knotholderentities.cpp
	# -------
	# Headers
	effect_ui.h
	lpe-fillet-chamfer.h
    parameter/parameter_ui.h
    parameter/nodesatellitesarray.h
    parameter/knotholderentities.h
)
# add_inkscape_lib(live_effects_LIB "${live_effects_SRC}")
add_inkscape_source("${live_effects_SRC}")
