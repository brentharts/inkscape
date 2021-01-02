#ifndef SEEN_GRADIENT_SELECTOR_INTERFACE_H
#define SEEN_GRADIENT_SELECTOR_INTERFACE_H

#include "object/sp-gradient.h"
#include "object/sp-gradient-spread.h"
#include "object/sp-gradient-units.h"

class GradientSelectorInterface {
public:
	enum SelectorMode { MODE_LINEAR, MODE_RADIAL, MODE_SWATCH };

	virtual SPGradient* getVector() = 0;
	virtual void setVector(SPDocument* doc, SPGradient* vector) = 0;
	virtual void setMode(SelectorMode mode) = 0;
	virtual void setUnits(SPGradientUnits units) = 0;
	virtual SPGradientUnits getUnits() = 0;
	virtual void setSpread(SPGradientSpread spread) = 0;
	virtual SPGradientSpread getSpread() = 0;
};

#endif
