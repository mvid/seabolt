/*
* Copyright (c) 2002-2018 "Neo4j,"
* Neo4j Sweden AB [http://neo4j.com]
*
* This file is part of Neo4j.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef SEABOLT_CONFIG
#define SEABOLT_CONFIG
#include <stddef.h>
#include <inttypes.h>

#include "bolt-version.h"
#include "bolt-exports.h"

#if USE_POSIXSOCK
#include <netdb.h>
#endif

#ifdef _WIN32
typedef unsigned short in_port_t;
typedef int socklen_t;
#endif // _WIN32

struct BoltLog;

#endif // SEABOLT_CONFIG