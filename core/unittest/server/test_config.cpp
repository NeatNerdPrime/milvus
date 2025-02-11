// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <gtest/gtest.h>
#include <gtest/gtest-death-test.h>

#include "config/YamlConfigMgr.h"
#include "server/Config.h"
#include "server/utils.h"
#include "utils/CommonUtil.h"
#include "utils/ValidationUtil.h"
#include "utils/StringHelpFunctions.h"

namespace {

static constexpr uint64_t KB = 1024;
static constexpr uint64_t MB = KB * 1024;
static constexpr uint64_t GB = MB * 1024;

} // namespace

TEST_F(ConfigTest, CONFIG_TEST) {
    milvus::server::ConfigMgr* config_mgr = milvus::server::YamlConfigMgr::GetInstance();

    milvus::Status s = config_mgr->LoadConfigFile("");
    ASSERT_FALSE(s.ok());

    std::string config_path(CONFIG_PATH);
    s = config_mgr->LoadConfigFile(config_path + INVALID_CONFIG_FILE);
    ASSERT_FALSE(s.ok());

    s = config_mgr->LoadConfigFile(config_path + VALID_CONFIG_FILE);
    ASSERT_TRUE(s.ok());

    config_mgr->Print();

    milvus::server::ConfigNode& root_config = config_mgr->GetRootNode();
    milvus::server::ConfigNode& server_config = root_config.GetChild("server_config");
    milvus::server::ConfigNode& db_config = root_config.GetChild("db_config");
    milvus::server::ConfigNode& metric_config = root_config.GetChild("metric_config");
    milvus::server::ConfigNode& cache_config = root_config.GetChild("cache_config");
    milvus::server::ConfigNode invalid_config = root_config.GetChild("invalid_config");
    auto valus = invalid_config.GetSequence("not_exist");
    float ff = invalid_config.GetFloatValue("not_exist", 3.0);
    ASSERT_EQ(ff, 3.0);

    std::string address = server_config.GetValue("address");
    ASSERT_TRUE(!address.empty());
    int64_t port = server_config.GetInt64Value("port");
    ASSERT_NE(port, 0);

    server_config.SetValue("test", "2.5");
    double test = server_config.GetDoubleValue("test");
    ASSERT_EQ(test, 2.5);

    milvus::server::ConfigNode fake;
    server_config.AddChild("fake", fake);
    fake = server_config.GetChild("fake");
    milvus::server::ConfigNodeArr arr;
    server_config.GetChildren(arr);
    ASSERT_EQ(arr.size(), 1UL);

    server_config.ClearChildren();
    auto children = server_config.GetChildren();
    ASSERT_TRUE(children.empty());

    server_config.ClearConfig();
    auto configs = server_config.GetConfig();
    ASSERT_TRUE(configs.empty());

    server_config.AddSequenceItem("seq", "aaa");
    server_config.AddSequenceItem("seq", "bbb");
    auto seq = server_config.GetSequence("seq");
    ASSERT_EQ(seq.size(), 2UL);

    milvus::server::ConfigNode combine;
    combine.Combine(server_config);

    combine.PrintAll();
    std::string all = combine.DumpString();
    ASSERT_TRUE(!all.empty());

    server_config.ClearSequences();
    auto seqs = server_config.GetSequences();
    ASSERT_TRUE(seqs.empty());
}

TEST_F(ConfigTest, SERVER_CONFIG_VALID_TEST) {
    std::string config_path(CONFIG_PATH);
    milvus::server::Config &config = milvus::server::Config::GetInstance();
    milvus::Status s;
    std::string str_val;
    int32_t int32_val;
    int64_t int64_val;
    float float_val;
    bool bool_val;

    /* server config */
    std::string server_addr = "192.168.1.155";
    s = config.SetServerConfigAddress(server_addr);
    ASSERT_TRUE(s.ok());
    s = config.GetServerConfigAddress(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == server_addr);

    std::string server_port = "12345";
    s = config.SetServerConfigPort(server_port);
    ASSERT_TRUE(s.ok());
    s = config.GetServerConfigPort(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == server_port);

    std::string server_mode = "cluster_readonly";
    s = config.SetServerConfigDeployMode(server_mode);
    ASSERT_TRUE(s.ok());
    s = config.GetServerConfigDeployMode(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == server_mode);

    std::string server_time_zone = "UTC+6";
    s = config.SetServerConfigTimeZone(server_time_zone);
    ASSERT_TRUE(s.ok());
    s = config.GetServerConfigTimeZone(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == server_time_zone);

    /* db config */
    std::string db_primary_path = "/home/zilliz";
    s = config.SetDBConfigPrimaryPath(db_primary_path);
    ASSERT_TRUE(s.ok());
    s = config.GetDBConfigPrimaryPath(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == db_primary_path);

    std::string db_secondary_path = "/home/zilliz";
    s = config.SetDBConfigSecondaryPath(db_secondary_path);
    ASSERT_TRUE(s.ok());
    s = config.GetDBConfigSecondaryPath(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == db_secondary_path);

    std::string db_backend_url = "mysql://root:123456@127.0.0.1:19530/milvus";
    s = config.SetDBConfigBackendUrl(db_backend_url);
    ASSERT_TRUE(s.ok());
    s = config.GetDBConfigBackendUrl(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == db_backend_url);

    int32_t db_archive_disk_threshold = 100;
    s = config.SetDBConfigArchiveDiskThreshold(std::to_string(db_archive_disk_threshold));
    ASSERT_TRUE(s.ok());
    s = config.GetDBConfigArchiveDiskThreshold(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == db_archive_disk_threshold);

    int32_t db_archive_days_threshold = 365;
    s = config.SetDBConfigArchiveDaysThreshold(std::to_string(db_archive_days_threshold));
    ASSERT_TRUE(s.ok());
    s = config.GetDBConfigArchiveDaysThreshold(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == db_archive_days_threshold);

    int32_t db_insert_buffer_size = 2;
    s = config.SetDBConfigInsertBufferSize(std::to_string(db_insert_buffer_size));
    ASSERT_TRUE(s.ok());
    s = config.GetDBConfigInsertBufferSize(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == db_insert_buffer_size);

    /* metric config */
    bool metric_enable_monitor = false;
    s = config.SetMetricConfigEnableMonitor(std::to_string(metric_enable_monitor));
    ASSERT_TRUE(s.ok());
    s = config.GetMetricConfigEnableMonitor(bool_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(bool_val == metric_enable_monitor);

    std::string metric_collector = "prometheus";
    s = config.SetMetricConfigCollector(metric_collector);
    ASSERT_TRUE(s.ok());
    s = config.GetMetricConfigCollector(str_val);
    ASSERT_TRUE(str_val == metric_collector);

    std::string metric_prometheus_port = "2222";
    s = config.SetMetricConfigPrometheusPort(metric_prometheus_port);
    ASSERT_TRUE(s.ok());
    s = config.GetMetricConfigPrometheusPort(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == metric_prometheus_port);

    /* cache config */
    int64_t cache_cpu_cache_capacity = 5;
    s = config.SetCacheConfigCpuCacheCapacity(std::to_string(cache_cpu_cache_capacity));
    ASSERT_TRUE(s.ok());
    s = config.GetCacheConfigCpuCacheCapacity(int64_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int64_val == cache_cpu_cache_capacity);

    float cache_cpu_cache_threshold = 0.1;
    s = config.SetCacheConfigCpuCacheThreshold(std::to_string(cache_cpu_cache_threshold));
    ASSERT_TRUE(s.ok());
    s = config.GetCacheConfigCpuCacheThreshold(float_val);
    ASSERT_TRUE(float_val == cache_cpu_cache_threshold);

    int64_t cache_gpu_cache_capacity = 1;
    s = config.SetCacheConfigGpuCacheCapacity(std::to_string(cache_gpu_cache_capacity));
    ASSERT_TRUE(s.ok());
    s = config.GetCacheConfigGpuCacheCapacity(int64_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int64_val == cache_gpu_cache_capacity);

    float cache_gpu_cache_threshold = 0.2;
    s = config.SetCacheConfigGpuCacheThreshold(std::to_string(cache_gpu_cache_threshold));
    ASSERT_TRUE(s.ok());
    s = config.GetCacheConfigGpuCacheThreshold(float_val);
    ASSERT_TRUE(float_val == cache_gpu_cache_threshold);

    bool cache_insert_data = true;
    s = config.SetCacheConfigCacheInsertData(std::to_string(cache_insert_data));
    ASSERT_TRUE(s.ok());
    s = config.GetCacheConfigCacheInsertData(bool_val);
    ASSERT_TRUE(bool_val == cache_insert_data);

    /* engine config */
    int32_t engine_use_blas_threshold = 50;
    s = config.SetEngineConfigUseBlasThreshold(std::to_string(engine_use_blas_threshold));
    ASSERT_TRUE(s.ok());
    s = config.GetEngineConfigUseBlasThreshold(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == engine_use_blas_threshold);

    int32_t engine_omp_thread_num = 8;
    s = config.SetEngineConfigOmpThreadNum(std::to_string(engine_omp_thread_num));
    ASSERT_TRUE(s.ok());
    s = config.GetEngineConfigOmpThreadNum(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == engine_omp_thread_num);

    int32_t engine_gpu_search_threshold = 800;
    s = config.SetEngineConfigGpuSearchThreshold(std::to_string(engine_gpu_search_threshold));
    ASSERT_TRUE(s.ok());
    s = config.GetEngineConfigGpuSearchThreshold(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == engine_gpu_search_threshold);

    /* resource config */
    std::string resource_mode = "simple";
    s = config.SetResourceConfigMode(resource_mode);
    ASSERT_TRUE(s.ok());
    s = config.GetResourceConfigMode(str_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(str_val == resource_mode);

    std::vector<std::string> search_resources = {"cpu", "gpu0"};
    std::vector<std::string> res_vec;
    std::string res_str;
    milvus::server::StringHelpFunctions::MergeStringWithDelimeter(search_resources,
            milvus::server::CONFIG_RESOURCE_SEARCH_RESOURCES_DELIMITER, res_str);
    s = config.SetResourceConfigSearchResources(res_str);
    ASSERT_TRUE(s.ok());
    s = config.GetResourceConfigSearchResources(res_vec);
    ASSERT_TRUE(s.ok());
    for (size_t i = 0; i < search_resources.size(); i++) {
        ASSERT_TRUE(search_resources[i] == res_vec[i]);
    }

    int32_t resource_index_build_device = 0;
    s = config.SetResourceConfigIndexBuildDevice("gpu" + std::to_string(resource_index_build_device));
    ASSERT_TRUE(s.ok());
    s = config.GetResourceConfigIndexBuildDevice(int32_val);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(int32_val == resource_index_build_device);
}

TEST_F(ConfigTest, SERVER_CONFIG_INVALID_TEST) {
    std::string config_path(CONFIG_PATH);
    milvus::server::Config& config = milvus::server::Config::GetInstance();
    milvus::Status s;

    s = config.LoadConfigFile("");
    ASSERT_FALSE(s.ok());

    s = config.LoadConfigFile(config_path + INVALID_CONFIG_FILE);
    ASSERT_FALSE(s.ok());
    s = config.LoadConfigFile(config_path + "dummy.yaml");
    ASSERT_FALSE(s.ok());

    /* server config */
    s = config.SetServerConfigAddress("0.0.0");
    ASSERT_FALSE(s.ok());
    s = config.SetServerConfigAddress("0.0.0.256");
    ASSERT_FALSE(s.ok());

    s = config.SetServerConfigPort("a");
    ASSERT_FALSE(s.ok());
    s = config.SetServerConfigPort("99999");
    ASSERT_FALSE(s.ok());

    s = config.SetServerConfigDeployMode("cluster");
    ASSERT_FALSE(s.ok());

    s = config.SetServerConfigTimeZone("GM");
    ASSERT_FALSE(s.ok());
    s = config.SetServerConfigTimeZone("GMT8");
    ASSERT_FALSE(s.ok());
    s = config.SetServerConfigTimeZone("UTCA");
    ASSERT_FALSE(s.ok());

    /* db config */
    s = config.SetDBConfigPrimaryPath("");
    ASSERT_FALSE(s.ok());

//    s = config.SetDBConfigSecondaryPath("");
//    ASSERT_FALSE(s.ok());

    s = config.SetDBConfigBackendUrl("http://www.google.com");
    ASSERT_FALSE(s.ok());
    s = config.SetDBConfigBackendUrl("sqlite://:@:");
    ASSERT_FALSE(s.ok());
    s = config.SetDBConfigBackendUrl("mysql://root:123456@127.0.0.1/milvus");
    ASSERT_FALSE(s.ok());

    s = config.SetDBConfigArchiveDiskThreshold("0x10");
    ASSERT_FALSE(s.ok());

    s = config.SetDBConfigArchiveDaysThreshold("0x10");
    ASSERT_FALSE(s.ok());

    s = config.SetDBConfigInsertBufferSize("a");
    ASSERT_FALSE(s.ok());
    s = config.SetDBConfigInsertBufferSize("0");
    ASSERT_FALSE(s.ok());
    s = config.SetDBConfigInsertBufferSize("2048");
    ASSERT_FALSE(s.ok());

    /* metric config */
    s = config.SetMetricConfigEnableMonitor("Y");
    ASSERT_FALSE(s.ok());

    s = config.SetMetricConfigCollector("zilliz");
    ASSERT_FALSE(s.ok());

    s = config.SetMetricConfigPrometheusPort("0xff");
    ASSERT_FALSE(s.ok());

    /* cache config */
    s = config.SetCacheConfigCpuCacheCapacity("a");
    ASSERT_FALSE(s.ok());
    s = config.SetCacheConfigCpuCacheCapacity("0");
    ASSERT_FALSE(s.ok());
    s = config.SetCacheConfigCpuCacheCapacity("2048");
    ASSERT_FALSE(s.ok());

    s = config.SetCacheConfigCpuCacheThreshold("a");
    ASSERT_FALSE(s.ok());
    s = config.SetCacheConfigCpuCacheThreshold("1.0");
    ASSERT_FALSE(s.ok());

    s = config.SetCacheConfigGpuCacheCapacity("a");
    ASSERT_FALSE(s.ok());
    s = config.SetCacheConfigGpuCacheCapacity("128");
    ASSERT_FALSE(s.ok());

    s = config.SetCacheConfigGpuCacheThreshold("a");
    ASSERT_FALSE(s.ok());
    s = config.SetCacheConfigGpuCacheThreshold("1.0");
    ASSERT_FALSE(s.ok());

    s = config.SetCacheConfigCacheInsertData("N");
    ASSERT_FALSE(s.ok());

    /* engine config */
    s = config.SetEngineConfigUseBlasThreshold("0xff");
    ASSERT_FALSE(s.ok());

    s = config.SetEngineConfigOmpThreadNum("a");
    ASSERT_FALSE(s.ok());
    s = config.SetEngineConfigOmpThreadNum("10000");
    ASSERT_FALSE(s.ok());

    s = config.SetEngineConfigGpuSearchThreshold("-1");
    ASSERT_FALSE(s.ok());

    /* resource config */
    s = config.SetResourceConfigMode("default");
    ASSERT_FALSE(s.ok());

    s = config.SetResourceConfigSearchResources("gpu10");
    ASSERT_FALSE(s.ok());
    s = config.SetResourceConfigSearchResources("cpu");
    ASSERT_FALSE(s.ok());

    s = config.SetResourceConfigIndexBuildDevice("gup2");
    ASSERT_FALSE(s.ok());
    s = config.SetResourceConfigIndexBuildDevice("gpu16");
    ASSERT_FALSE(s.ok());
}

TEST_F(ConfigTest, SERVER_CONFIG_TEST) {
    std::string config_path(CONFIG_PATH);
    milvus::server::Config& config = milvus::server::Config::GetInstance();
    milvus::Status s = config.LoadConfigFile(config_path + VALID_CONFIG_FILE);
    ASSERT_TRUE(s.ok());

    s = config.ValidateConfig();
    ASSERT_TRUE(s.ok());

    config.PrintAll();

    s = config.ResetDefaultConfig();
    ASSERT_TRUE(s.ok());
}

