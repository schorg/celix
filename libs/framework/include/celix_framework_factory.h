/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef CELIX_FRAMEWORK_FACTORY_H_
#define CELIX_FRAMEWORK_FACTORY_H_

#include "celix_framework.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new framework. The framework will be in the started state.
 * @param config The framework configuration. Can be NULL.
 * @return a started framework or NULL
 */
CELIX_FRAMEWORK_EXPORT celix_framework_t* celix_frameworkFactory_createFramework(celix_properties_t *config);

/**
 * Stop and destroys a Celix framework
 * Note that this call block till the Celix framework is stopped
 */
CELIX_FRAMEWORK_EXPORT void celix_frameworkFactory_destroyFramework(celix_framework_t *fw);


#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_FACTORY_H_ */
