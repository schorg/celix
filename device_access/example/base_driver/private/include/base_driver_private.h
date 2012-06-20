/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef BASE_DRIVER_PRIVATE_H_
#define BASE_DRIVER_PRIVATE_H_

#include "base_driver_device.h"

celix_status_t baseDriver_create(apr_pool_t *pool, base_driver_device_t *service);
celix_status_t baseDriver_createService(base_driver_device_t device, base_driver_device_service_t *service);

celix_status_t baseDriver_noDriverFound(device_t device);

celix_status_t baseDriver_getNextChar(base_driver_device_t service, char *c);

#endif /* BASE_DRIVER_PRIVATE_H_ */
