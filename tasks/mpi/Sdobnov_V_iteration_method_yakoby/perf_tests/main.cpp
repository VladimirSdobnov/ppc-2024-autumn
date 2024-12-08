#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <random>
#include <vector>

#include "core/perf/include/perf.hpp"
#include "mpi/Sdobnov_V_iteration_method_yakoby/include/ops_mpi.hpp"

std::pair<std::vector<double>, std::vector<double>> generate_diagonally_dominant_matrix(int n, double min_val = -10.0,
                                                                                        double max_val = 10.0) {
  std::vector<double> A(n * n);
  std::vector<double> b(n);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(min_val, max_val);
  for (int i = 0; i < n; ++i) {
    double row_sum = 0.0;
    for (int j = 0; j < n; ++j) {
      if (i != j) {
        A[i * n + j] = dist(gen);
        row_sum += std::abs(A[i * n + j]);
      }
    }
    A[i * n + i] = row_sum + std::abs(dist(gen)) + 1.0;
    b[i] = dist(gen);
  }
  return {A, b};
}

TEST(Sdobnov_V_iteration_method_yakoby_par, test_pipeline_run) {
  boost::mpi::communicator world;
  const size_t size = 1000;
  auto [matrix, free_members] = generate_diagonally_dominant_matrix(size);
  std::vector<double> res(size, 0.0);

  std::shared_ptr<ppc::core::TaskData> taskDataPar = std::make_shared<ppc::core::TaskData>();
  if (world.rank() == 0) {
    taskDataPar->inputs_count.emplace_back(size);
    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(matrix.data()));
    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(free_members.data()));
    taskDataPar->outputs_count.emplace_back(size);
    taskDataPar->outputs.emplace_back(reinterpret_cast<uint8_t *>(res.data()));
  }

  auto test = std::make_shared<Sdobnov_iteration_method_yakoby::IterationMethodYakobyPar>(taskDataPar);
  ASSERT_EQ(test->validation(), true);
  test->pre_processing();
  test->run();
  test->post_processing();

  auto perfAttr = std::make_shared<ppc::core::PerfAttr>();
  perfAttr->num_running = 10;
  const boost::mpi::timer current_timer;
  perfAttr->current_timer = [&] { return current_timer.elapsed(); };

  auto perfResults = std::make_shared<ppc::core::PerfResults>();
  auto perfAnalyzer = std::make_shared<ppc::core::Perf>(test);
  perfAnalyzer->pipeline_run(perfAttr, perfResults);
  if (world.rank() == 0) {
    ppc::core::Perf::print_perf_statistic(perfResults);
    ASSERT_EQ(size, res.size());
  }
}

TEST(Sdobnov_V_iteration_method_yakoby_par, test_task_run) {
  boost::mpi::communicator world;
  const size_t size = 1000;
  auto [matrix, free_members] = generate_diagonally_dominant_matrix(size);
  std::vector<double> res(size, 0.0);

  std::shared_ptr<ppc::core::TaskData> taskDataPar = std::make_shared<ppc::core::TaskData>();
  if (world.rank() == 0) {
    taskDataPar->inputs_count.emplace_back(size);
    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(matrix.data()));
    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(free_members.data()));
    taskDataPar->outputs_count.emplace_back(size);
    taskDataPar->outputs.emplace_back(reinterpret_cast<uint8_t *>(res.data()));
  }

  auto test = std::make_shared<Sdobnov_iteration_method_yakoby::IterationMethodYakobyPar>(taskDataPar);
  ASSERT_EQ(test->validation(), true);
  test->pre_processing();
  test->run();
  test->post_processing();

  auto perfAttr = std::make_shared<ppc::core::PerfAttr>();
  perfAttr->num_running = 10;
  const boost::mpi::timer current_timer;
  perfAttr->current_timer = [&] { return current_timer.elapsed(); };

  auto perfResults = std::make_shared<ppc::core::PerfResults>();
  auto perfAnalyzer = std::make_shared<ppc::core::Perf>(test);
  perfAnalyzer->task_run(perfAttr, perfResults);
  if (world.rank() == 0) {
    ppc::core::Perf::print_perf_statistic(perfResults);
    ASSERT_EQ(size, res.size());
  }
}