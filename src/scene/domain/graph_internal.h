/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Graph internals                                                                              */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

#define DVZ_GRAPH_COMPOSITE_EDGES_ROLE "edges"
#define DVZ_GRAPH_COMPOSITE_NODES_ROLE "nodes"
#define DVZ_GRAPH_COMPOSITE_SEGMENTS_INTERNAL_ROLE "_edges_segment"
#define DVZ_GRAPH_COMPOSITE_PATH_INTERNAL_ROLE "_edges_path"

DvzGraph* _scene_alloc_graph(DvzScene* scene);

void _scene_graph_reset(DvzGraph* graph);

void _graph_mark_composites_dirty(DvzGraph* graph, bool nodes_dirty, bool edges_dirty);

int _graph_composite_prepare(DvzComposite* composite);
