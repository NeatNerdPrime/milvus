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

#include <list>
#include <random>
#include "../Algorithm.h"
#include "Action.h"
#include "scheduler/tasklabel/SpecResLabel.h"
#include "src/cache/GpuCacheMgr.h"
#include "src/server/Config.h"

namespace milvus {
namespace scheduler {

std::vector<ResourcePtr>
get_neighbours(const ResourcePtr& self) {
    std::vector<ResourcePtr> neighbours;
    for (auto& neighbour_node : self->GetNeighbours()) {
        auto node = neighbour_node.neighbour_node;
        if (not node)
            continue;

        auto resource = std::static_pointer_cast<Resource>(node);
        //        if (not resource->HasExecutor()) continue;

        neighbours.emplace_back(resource);
    }
    return neighbours;
}

std::vector<std::pair<ResourcePtr, Connection>>
get_neighbours_with_connetion(const ResourcePtr& self) {
    std::vector<std::pair<ResourcePtr, Connection>> neighbours;
    for (auto& neighbour_node : self->GetNeighbours()) {
        auto node = neighbour_node.neighbour_node;
        if (not node)
            continue;

        auto resource = std::static_pointer_cast<Resource>(node);
        //        if (not resource->HasExecutor()) continue;
        Connection conn = neighbour_node.connection;
        neighbours.emplace_back(std::make_pair(resource, conn));
    }
    return neighbours;
}

void
Action::PushTaskToNeighbourRandomly(TaskTableItemPtr task_item, const ResourcePtr& self) {
    auto neighbours = get_neighbours_with_connetion(self);
    if (not neighbours.empty()) {
        std::vector<uint64_t> speeds;
        uint64_t total_speed = 0;
        for (auto& neighbour : neighbours) {
            uint64_t speed = neighbour.second.speed();
            speeds.emplace_back(speed);
            total_speed += speed;
        }

        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 mt(seed1);
        std::uniform_int_distribution<int> dist(0, total_speed);
        uint64_t index = 0;
        int64_t rd_speed = dist(mt);
        for (uint64_t i = 0; i < speeds.size(); ++i) {
            rd_speed -= speeds[i];
            if (rd_speed <= 0) {
                neighbours[i].first->task_table().Put(task_item->task, task_item);
                return;
            }
        }

    } else {
        // TODO(wxyu): process
    }
}

void
Action::PushTaskToAllNeighbour(TaskTableItemPtr task_item, const ResourcePtr& self) {
    auto neighbours = get_neighbours(self);
    for (auto& neighbour : neighbours) {
        neighbour->task_table().Put(task_item->task, task_item);
    }
}

void
Action::PushTaskToResource(TaskTableItemPtr task_item, const ResourcePtr& dest) {
    dest->task_table().Put(task_item->task, task_item);
}

void
Action::DefaultLabelTaskScheduler(const ResourceMgrPtr& res_mgr, ResourcePtr resource,
                                  std::shared_ptr<LoadCompletedEvent> event) {
    if (not resource->HasExecutor() && event->task_table_item_->Move()) {
        auto task_item = event->task_table_item_;
        auto task = event->task_table_item_->task;
        auto search_task = std::static_pointer_cast<XSearchTask>(task);
        bool moved = false;

        // to support test task, REFACTOR
        if (resource->type() == ResourceType::CPU) {
            if (auto index_engine = search_task->index_engine_) {
                auto location = index_engine->GetLocation();

                for (auto i = 0; i < res_mgr->GetNumGpuResource(); ++i) {
                    auto index = milvus::cache::GpuCacheMgr::GetInstance(i)->GetIndex(location);
                    if (index != nullptr) {
                        moved = true;
                        auto dest_resource = res_mgr->GetResource(ResourceType::GPU, i);
                        PushTaskToResource(event->task_table_item_, dest_resource);
                        break;
                    }
                }
            }
        }

        if (not moved) {
            PushTaskToNeighbourRandomly(task_item, resource);
        }
    }
}

void
Action::SpecifiedResourceLabelTaskScheduler(const ResourceMgrPtr& res_mgr, ResourcePtr resource,
                                            std::shared_ptr<LoadCompletedEvent> event) {
    auto task_item = event->task_table_item_;
    auto task = event->task_table_item_->task;
    //    if (resource->type() == ResourceType::DISK) {
    //        // step 1: calculate shortest path per resource, from disk to compute resource
    //        auto compute_resources = res_mgr->GetComputeResources();
    //        std::vector<std::vector<std::string>> paths;
    //        std::vector<uint64_t> transport_costs;
    //        for (auto& res : compute_resources) {
    //            std::vector<std::string> path;
    //            uint64_t transport_cost = ShortestPath(resource, res, res_mgr, path);
    //            transport_costs.push_back(transport_cost);
    //            paths.emplace_back(path);
    //        }
    //        if (task->job_.lock()->type() == JobType::BUILD) {
    //            // step2: Read device id in config
    //            // get build index gpu resource
    //            server::Config& config = server::Config::GetInstance();
    //            int32_t build_index_gpu;
    //            Status stat = config.GetResourceConfigIndexBuildDevice(build_index_gpu);
    //
    //            bool find_gpu_res = false;
    //            if (res_mgr->GetResource(ResourceType::GPU, build_index_gpu) != nullptr) {
    //                for (uint64_t i = 0; i < compute_resources.size(); ++i) {
    //                    if (compute_resources[i]->name() ==
    //                        res_mgr->GetResource(ResourceType::GPU, build_index_gpu)->name()) {
    //                        find_gpu_res = true;
    //                        Path task_path(paths[i], paths[i].size() - 1);
    //                        task->path() = task_path;
    //                        break;
    //                    }
    //                }
    //            }
    //            if (not find_gpu_res) {
    //                task->path() = Path(paths[0], paths[0].size() - 1);
    //            }
    //        }
    //    }

    if (resource->name() == task->path().Last()) {
        resource->WakeupExecutor();
    } else {
        auto next_res_name = task->path().Next();
        auto next_res = res_mgr->GetResource(next_res_name);
        //        if (event->task_table_item_->Move()) {
        //            next_res->task_table().Put(task);
        //        }
        event->task_table_item_->Move();
        next_res->task_table().Put(task, task_item);
    }
}

}  // namespace scheduler
}  // namespace milvus
