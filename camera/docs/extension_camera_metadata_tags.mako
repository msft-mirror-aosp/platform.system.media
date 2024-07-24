## -*- coding: utf-8 -*-
/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vector>
#pragma once
/**
 * ! Do not edit this file directly !
 *
 * Generated automatically from extensions_camera_metadata_tags.mako. To be included in libcameraservice
 * only by aidl/AidlUtils.cpp.
 */

/**
 * API level to dynamic keys mapping. To be used for filtering out keys depending on vndk version
 * used by vendor clients.
 */
std::vector<camera_metadata_tag> extension_metadata_keys{
% for sec in find_all_sections(metadata):
    % if sec.name in ("efv", "extension"):
      % for entry in find_unique_entries(sec):
        % if entry.kind == 'dynamic':
            ${entry.name | csym},
        %endif
      %endfor
    %endif
% endfor
};
