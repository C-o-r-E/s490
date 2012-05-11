/*
   Copyright 2012 Corey Clayton, Eric Chan, Mathieu Dumais-Savard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <map>

#include "RunCondition.h"
#include "common/DataPoint.h"

RunCondition::RunCondition(bbtag t) {
    tag = t;
}

RunCondition::~RunCondition() {
}

bool RunCondition::canRun(const std::map<bbtag, DataPoint>& tagmap) {
    std::map<bbtag, DataPoint>::const_iterator it = tagmap.find(tag);

    if (it == tagmap.end() || it->second.size() < sizeof (int)) {
        return false;
    }

    return (it->second.unwrap<int>() != 0);
}
