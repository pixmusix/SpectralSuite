#include "utilities.h"

namespace utilities {

	int ms2Samps(int ms, int sRate) {
		return ms * (sRate / 1000);
	}

	int wrap(int index, int limit) {
		if (index < 0) {
			return index + limit;
		}
		else {
			return index % limit;
		}
	}	
}