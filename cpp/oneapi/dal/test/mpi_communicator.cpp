/*******************************************************************************
* Copyright 2021-2022 Intel Corporation
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
*******************************************************************************/

#include "oneapi/dal/test/engine/mpi_global.hpp"
#include "oneapi/dal/test/engine/fixtures.hpp"

namespace spmd = oneapi::dal::preview::spmd;

namespace oneapi::dal::test {

namespace te = dal::test::engine;

class mpi_comm_test : public te::policy_fixture {
public:
#ifdef ONEDAL_DATA_PARALLEL
    using comm_t = spmd::communicator<spmd::device_memory_access::usm>;
#else
    using comm_t = spmd::communicator<spmd::device_memory_access::none>;
#endif

    comm_t get_new_comm() {
#ifdef ONEDAL_DATA_PARALLEL
        return te::get_global_mpi_device_communicator(get_queue());
#else
        return te::get_global_mpi_host_communicator();
#endif
    }

    template <typename T>
    void test_bcast(T* buffer, std::int64_t count) {
        auto comm = get_new_comm();
        comm.bcast(buffer, count).wait();
    }

#ifdef ONEDAL_DATA_PARALLEL
    template <typename T>
    void test_bcast_on_device(T* buffer, std::int64_t count) {
        auto buffer_device = copy_to_device(buffer, count);
        auto comm = get_new_comm();
        comm.bcast(get_queue(), buffer_device.get_mutable_data(), count).wait();
        copy_to_host(buffer, buffer_device.get_data(), count);
    }
#endif

    template <typename T>
    void test_allreduce(T* buffer, std::int64_t count) {
        get_new_comm().allreduce(buffer, buffer, count, spmd::reduce_op::sum).wait();
    }

#ifdef ONEDAL_DATA_PARALLEL
    template <typename T>
    void test_allreduce_on_device(T* buffer, std::int64_t count) {
        auto buffer_device = copy_to_device(buffer, count);
        get_new_comm()
            .allreduce(get_queue(),
                       buffer_device.get_mutable_data(),
                       buffer_device.get_mutable_data(),
                       count,
                       spmd::reduce_op::sum)
            .wait();
        copy_to_host(buffer, buffer_device.get_data(), count);
    }
#endif

    template <typename T>
    void test_allgatherv(T* send_buffer,
                         std::int64_t send_count,
                         T* recv_buffer,
                         std::int64_t* recv_counts,
                         std::int64_t* displs) {
        get_new_comm().allgatherv(send_buffer, send_count, recv_buffer, recv_counts, displs).wait();
    }

#ifdef ONEDAL_DATA_PARALLEL
    template <typename T>
    void test_allgatherv_on_device(T* send_buf,
                                   std::int64_t send_count,
                                   T* recv_buf,
                                   std::int64_t* recv_counts,
                                   std::int64_t* displs) {
        auto comm = get_new_comm();
        auto send_buffer_device = copy_to_device(send_buf, send_count);
        std::int64_t total_count = 0;
        for (std::int64_t i = 0; i < comm.get_rank_count(); i++) {
            total_count += recv_counts[i];
        }
        auto recv_buffer_device =
            array<T>::empty(get_queue(), total_count, sycl::usm::alloc::device);
        comm.allgatherv(get_queue(),
                        send_buffer_device.get_mutable_data(),
                        send_count,
                        recv_buffer_device.get_mutable_data(),
                        recv_counts,
                        displs)
            .wait();
        copy_to_host(recv_buf, recv_buffer_device.get_data(), total_count);
    }
#endif

private:
#ifdef ONEDAL_DATA_PARALLEL
    template <typename T>
    array<T> copy_to_device(const T* data, std::int64_t count) {
        auto x = array<T>::empty(get_queue(), count, sycl::usm::alloc::device);
        dal::detail::memcpy_host2usm(get_queue(), x.get_mutable_data(), data, sizeof(T) * count);
        return x;
    }
#endif

#ifdef ONEDAL_DATA_PARALLEL
    template <typename T>
    void copy_to_host(T* dst, const T* src, std::int64_t count) {
        dal::detail::memcpy_usm2host(get_queue(), dst, src, sizeof(T) * count);
    }
#endif
};

TEST_M(mpi_comm_test, "bcast") {
    constexpr std::int64_t count = 100;

    float buffer[count] = { 0.0 };
    if (get_new_comm().is_root_rank()) {
        for (std::int64_t i = 0; i < count; i++) {
            buffer[i] = float(i);
        }
    }

    SECTION("host") {
        test_bcast(buffer, count);
    }

#ifdef ONEDAL_DATA_PARALLEL
    SECTION("device") {
        test_bcast_on_device(buffer, count);
    }
#endif

    for (std::int64_t i = 0; i < count; i++) {
        REQUIRE(buffer[i] == float(i));
    }
}

TEST_M(mpi_comm_test, "allreduce") {
    constexpr std::int64_t count = 100;

    float buffer[count];
    for (std::int64_t i = 0; i < count; i++) {
        buffer[i] = 1.0f;
    }

    SECTION("host") {
        test_allreduce(buffer, count);
    }

#ifdef ONEDAL_DATA_PARALLEL
    SECTION("device") {
        test_allreduce_on_device(buffer, count);
    }
#endif

    const std::int64_t rank_count = get_new_comm().get_rank_count();
    for (std::int64_t i = 0; i < count; i++) {
        REQUIRE(buffer[i] == float(rank_count));
    }
}

TEST_M(mpi_comm_test, "allgatherv") {
    auto comm = get_new_comm();
    const std::int64_t granularity = 10;
    const std::int64_t rank_count = comm.get_rank_count();
    const std::int64_t rank = comm.get_rank();

    std::vector<std::int64_t> recv_counts(rank_count);
    std::vector<std::int64_t> displs(rank_count);
    std::int64_t total_size = 0;
    for (std::int64_t i = 0; i < rank_count; i++) {
        recv_counts[i] = (i + 1) * granularity;
        displs[i] = total_size;
        total_size += recv_counts[i];
    }

    const std::int64_t rank_size = recv_counts[rank];
    std::vector<float> send_buffer(rank_size);
    for (std::int64_t i = 0; i < rank_size; i++) {
        send_buffer[i] = float(rank);
    }

    std::vector<float> recv_buffer(total_size);
    std::vector<float> final_buffer(total_size);
    std::int64_t offset = 0;
    for (std::int64_t i = 0; i < rank_count; i++) {
        for (std::int64_t j = 0; j < recv_counts[i]; j++) {
            final_buffer[offset] = float(i);
            offset++;
        }
    }

    SECTION("host") {
        test_allgatherv(send_buffer.data(),
                        rank_size,
                        recv_buffer.data(),
                        recv_counts.data(),
                        displs.data());
    }

#ifdef ONEDAL_DATA_PARALLEL
    SECTION("device") {
        test_allgatherv_on_device(send_buffer.data(),
                                  rank_size,
                                  recv_buffer.data(),
                                  recv_counts.data(),
                                  displs.data());
    }
#endif

    for (std::int64_t i = 0; i < total_size; i++) {
        REQUIRE(recv_buffer[i] == final_buffer[i]);
    }
}

TEST_M(mpi_comm_test, "allgatherv_arbitrary_displacements") {
    auto comm = get_new_comm();
    const std::int64_t granularity = 10;
    const std::int64_t rank_count = comm.get_rank_count();
    const std::int64_t rank = comm.get_rank();

    std::vector<std::int64_t> recv_counts(rank_count);
    std::vector<std::int64_t> displs(rank_count);
    std::int64_t total_size = 0;
    for (std::int64_t i = 0; i < rank_count; i++) {
        recv_counts[i] = (i + 2) * granularity;
        displs[i] = total_size;
        total_size += recv_counts[i];
    }

    const std::int64_t rank_size = recv_counts[rank];
    std::vector<float> send_buffer(rank_size);
    for (std::int64_t i = 0; i < rank_size - 10; i++) {
        send_buffer[i] = float(rank);
    }
    std::vector<float> recv_buffer(total_size);
    std::vector<float> final_buffer(total_size);
    std::int64_t offset = 0;
    for (std::int64_t i = 0; i < rank_count; i++) {
        for (std::int64_t j = 0; j < recv_counts[i] - 10; j++) {
            final_buffer[offset] = float(i);
            offset++;
            if (j == (recv_counts[i] - 11)) {
                offset += 10;
            }
        }
    }

    SECTION("host") {
        test_allgatherv(send_buffer.data(),
                        rank_size,
                        recv_buffer.data(),
                        recv_counts.data(),
                        displs.data());
    }

#ifdef ONEDAL_DATA_PARALLEL
    SECTION("device") {
        test_allgatherv_on_device(send_buffer.data(),
                                  rank_size,
                                  recv_buffer.data(),
                                  recv_counts.data(),
                                  displs.data());
    }
#endif

    for (std::int64_t i = 0; i < total_size; i++) {
        REQUIRE(recv_buffer[i] == final_buffer[i]);
    }
}

} // namespace oneapi::dal::test
