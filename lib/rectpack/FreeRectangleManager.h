#ifndef RECTPACK_FREERECTANGLEMANAGER_H
#define RECTPACK_FREERECTANGLEMANAGER_H

#include <memory>
#include "FreeRectangleManager.h"

#define FRM_HAS_REFERENCE 0

namespace rbp {

struct Rect;

class FreeRectangleManager {
public:
	FreeRectangleManager(const Rect& root);
	~FreeRectangleManager();

	void PlaceRect(const Rect &node);

	size_t size() const;
	const Rect& getFree(size_t off) const;
private:
	struct InternalData;
	std::shared_ptr<InternalData> d_;
};

}

#endif
