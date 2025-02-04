/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include "bundle_archive_private.h"
#include "celix_bundle_cache.h"
#include "celix_constants.h"
#include "celix_file_utils.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "framework_private.h"
#include "gtest/gtest.h"
#include <string>

class CelixBundleCacheTestSuite : public ::testing::Test {
public:
    CelixBundleCacheTestSuite() {
        celix_bundle_cache_t* bundleCache = nullptr;
        fw.configurationMap = celix_properties_create();
        celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
        fw.logger = celix_frameworkLogger_create(CELIX_LOG_LEVEL_TRACE);
        EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &bundleCache));
        fw.cache = bundleCache;
    }

    ~CelixBundleCacheTestSuite() override {
        EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(fw.cache));
        celix_frameworkLogger_destroy(fw.logger);
        celix_properties_destroy(fw.configurationMap);
    }

    struct celix_framework fw{};
};

TEST_F(CelixBundleCacheTestSuite, ArchiveCreateDestroyTest) {
    bundle_archive_t* archive = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_createArchive(fw.cache, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_NE(nullptr, archive);
    auto location = celix_bundleArchive_getLocation(archive);
    EXPECT_STREQ(SIMPLE_TEST_BUNDLE1_LOCATION, location);
    free(location);
    EXPECT_EQ(1, celix_bundleCache_findBundleIdForLocation(fw.cache, SIMPLE_TEST_BUNDLE1_LOCATION));
    EXPECT_TRUE(celix_bundleCache_isBundleIdAlreadyUsed(fw.cache, 1));
    std::string loc = celix_bundleArchive_getPersistentStoreRoot(archive);
    EXPECT_TRUE(celix_utils_directoryExists(loc.c_str()));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroyArchive(fw.cache, archive));
    EXPECT_EQ(-1, celix_bundleCache_findBundleIdForLocation(fw.cache, SIMPLE_TEST_BUNDLE1_LOCATION));
    EXPECT_FALSE(celix_bundleCache_isBundleIdAlreadyUsed(fw.cache, 1));
    EXPECT_FALSE(celix_utils_directoryExists(loc.c_str()));
}

TEST_F(CelixBundleCacheTestSuite, SystemArchiveCreateDestroyTest) {
    bundle_archive_t* archive = nullptr;
    const char* archiveRoot = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_createSystemArchive(&fw, &archive));
    EXPECT_NE(nullptr, archive);
    EXPECT_EQ(0, celix_bundleArchive_getId(archive));
    EXPECT_EQ(CELIX_SUCCESS, bundleArchive_getArchiveRoot(archive, &archiveRoot));
    EXPECT_EQ(nullptr, archiveRoot);
    EXPECT_EQ(nullptr, celix_bundleArchive_getLocation(archive));
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroyArchive(fw.cache, archive));
}

TEST_F(CelixBundleCacheTestSuite, CreateBundleArchivesCacheTest) {
    celix_properties_set(fw.configurationMap, CELIX_AUTO_START_1, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(fw.configurationMap, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);
    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_createBundleArchivesCache(&fw, true));
    EXPECT_EQ(1, celix_bundleCache_findBundleIdForLocation(fw.cache, SIMPLE_TEST_BUNDLE1_LOCATION));
    EXPECT_TRUE(celix_bundleCache_isBundleIdAlreadyUsed(fw.cache, 1));
    EXPECT_EQ(2, celix_bundleCache_findBundleIdForLocation(fw.cache, SIMPLE_TEST_BUNDLE2_LOCATION));
    EXPECT_TRUE(celix_bundleCache_isBundleIdAlreadyUsed(fw.cache, 2));
}