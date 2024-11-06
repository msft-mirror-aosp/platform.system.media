## -*- coding: utf-8 -*-
/*
 * Copyright (C) ${copyright_year()} The Android Open Source Project
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
<%!
  def annotated_type(entry):
    if entry.enum:
       type = 'enum'
    else:
       type = entry.type
    if entry.container == 'array':
       type += '[]'

    return type

  def annotated_enum_type(entry):
    if entry.type == 'int64' and entry.container == 'array':
       type = ' int64_t'
    else:
       type = ' uint32_t'

    return type
%>\

/*
 * Autogenerated from camera metadata definitions in
 * /system/media/camera/docs/metadata_definitions.xml
 * *** DO NOT EDIT BY HAND ***
 */

package android.hardware.camera.metadata;

import android.hardware.camera.metadata.CameraMetadataSectionStart;

/**
 * Main enumeration for defining camera metadata tags added in this revision
 *
 * <p>Partial documentation is included for each tag; for complete documentation, reference
 * '/system/media/camera/docs/docs.html' in the corresponding Android source tree.</p>
 */
@VintfStability
@Backing(type="int")
enum CameraMetadataTag {
<% curIdx = 0 %>\
<% gap = False %>\
% for sec_idx,sec in enumerate(find_all_sections_filtered(metadata, ('extension'))):
  % for idx,entry in enumerate(remove_synthetic(find_unique_entries(sec))):
    % if idx == 0:
<% gap = False %>\
<% curIdx = sec_idx << 16 %>\
    % endif
    % if entry.visibility in ('fwk_only', 'fwk_java_public'):
<% gap = True %>\
<% curIdx += 1 %>\
<% continue %>\
    % endif
    /**
     * ${entry.name} [${entry.kind}, ${annotated_type(entry)}, ${entry.applied_visibility}]
    % if entry.description:
     *
${entry.description | hidldoc(metadata)}\
    % endif
     */
    % if idx == 0:
    ${entry.name + " =" | csym} CameraMetadataSectionStart.${path_name(find_parent_section(entry)) | csym}_START,
    % elif gap:
<% gap = False %>\
    ${entry.name | csym} = ${curIdx},
    % else:
    ${entry.name + "," | csym}
    % endif
<% curIdx += 1 %>\
  % endfor
%endfor
}
