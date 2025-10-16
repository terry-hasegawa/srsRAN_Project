/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_low_config_cli11_schema.h"
#include "apps/helpers/logger/logger_appconfig_cli11_utils.h"
#include "apps/helpers/metrics/metrics_config_cli11_schema.h"
#include "apps/services/worker_manager/cli11_cpu_affinities_parser_helper.h"
#include "du_low_config.h"
#include "srsran/adt/expected.h"
#include "srsran/ran/slot_point.h"
#include "srsran/ran/slot_point_extended.h"
#include "srsran/support/cli11_utils.h"
#include "srsran/support/config_parsers.h"

using namespace srsran;

template <typename Integer>
static expected<Integer, std::string> parse_int(const std::string& value)
{
  try {
    return std::stoi(value);
  } catch (const std::invalid_argument& e) {
    return make_unexpected(e.what());
  } catch (const std::out_of_range& e) {
    return make_unexpected(e.what());
  }
}

static void configure_cli11_log_args(CLI::App& app, du_low_unit_logger_config& log_params)
{
  app_helpers::add_log_option(app, log_params.phy_level, "--phy_level", "PHY log level");
  app_helpers::add_log_option(app, log_params.hal_level, "--hal_level", "HAL log level");

  add_option(app,
             "--broadcast_enabled",
             log_params.broadcast_enabled,
             "Enable logging in the physical and MAC layer of broadcast messages and all PRACH opportunities")
      ->always_capture_default();
  app.add_option("--phy_rx_symbols_filename",
                 log_params.phy_rx_symbols_filename,
                 "Set to a valid file path to print the received symbols.")
      ->always_capture_default();
  app.add_option_function<std::string>(
         "--phy_rx_symbols_port",
         [&log_params](const std::string& value) {
           if (value == "all") {
             log_params.phy_rx_symbols_port = std::nullopt;
           } else {
             log_params.phy_rx_symbols_port = parse_int<unsigned>(value).value();
           }
         },
         "Set to a valid receive port number to dump the IQ symbols from that port only, or set to \"all\" to dump the "
         "IQ symbols from all UL receive ports. Only works if \"phy_rx_symbols_filename\" is set.")
      ->default_str("0")
      ->check(CLI::NonNegativeNumber | CLI::IsMember({"all"}));
  app.add_option("--phy_rx_symbols_prach",
                 log_params.phy_rx_symbols_prach,
                 "Set to true to dump the IQ symbols from all the PRACH ports. Only works if "
                 "\"phy_rx_symbols_filename\" is set.")
      ->capture_default_str();

  add_option(
      app, "--hex_max_size", log_params.hex_max_size, "Maximum number of bytes to print in hex (zero for no hex dumps)")
      ->capture_default_str()
      ->check(CLI::Range(0, 1024));
}

static void configure_cli11_trace_args(CLI::App& app, du_low_unit_tracer_config& config)
{
  CLI::App* layers_subcmd = add_subcommand(app, "layers", "Layer basis tracing configuration")->configurable();
  add_option(
      *layers_subcmd, "--phy_enable", config.executor_tracing_enable, "Enable tracing for physical layer executors")
      ->capture_default_str();
}

static void configure_cli11_upper_phy_threads_args(CLI::App& app, du_low_unit_expert_threads_config& config)
{
  auto pdsch_processor_check = [](const std::string& value) -> std::string {
    if ((value == "auto") || (value == "generic") || (value == "flexible")) {
      return {};
    }
    return "Invalid PDSCH processor type. Accepted values [auto,generic,flexible]";
  };

  auto pdsch_cb_batch_length_transform = [](const std::string& value) -> std::string {
    unsigned pdsch_cb_batch_length;
    if ((value == "auto") || (value == "default")) {
      pdsch_cb_batch_length = du_low_unit_expert_threads_config::default_cb_batch_length;
    } else if (value == "synchronous") {
      pdsch_cb_batch_length = du_low_unit_expert_threads_config::synchronous_cb_batch_length;
    } else {
      char* val             = nullptr;
      pdsch_cb_batch_length = std::strtol(value.c_str(), &val, 10);
      if (val != value.c_str() + value.length()) {
        return fmt::format("Invalid PDSCH CB batch size '{}'. Set to auto, synchronous, or an integer number.", value);
      }
    }
    return std::to_string(pdsch_cb_batch_length);
  };

  add_option(
      app, "--pdsch_processor_type", config.pdsch_processor_type, "PDSCH processor type: auto, generic and flexible.")
      ->capture_default_str()
      ->check(pdsch_processor_check);
  add_option(app,
             "--pdsch_cb_batch_length",
             config.pdsch_cb_batch_length,
             "PDSCH flexible processor codeblock-batch size.\n"
             "Set it to 'auto' to adapt the batch length to the number of threads dedicated to downlink processing,\n"
             "set it to 'synchronous' to disable batch-splitting and ensure that TB processing remains within the \n"
             "calling thread without parallelization.")
      ->capture_default_str()
      ->transform(pdsch_cb_batch_length_transform);
  add_option(app,
             "--max_pucch_concurrency",
             config.max_pucch_concurrency,
             "Maximum PUCCH processing concurrency for all cells.\n"
             "Limits the maximum number of threads that can concurrently process Physical Uplink Control Channel\n"
             "(PUCCH). Set it to zero for no limit of threads.")
      ->capture_default_str()
      ->check(CLI::Number);
  add_option(app,
             "--max_pusch_and_srs_concurrency",
             config.max_pusch_and_srs_concurrency,
             "Maximum PUSCH and SRS processing concurrency for all cells.\n"
             "Limits the maximum number of threads that can concurrently process Physical Uplink Shared Channel \n"
             "(PUSCH) and Sounding Reference Signals (SRS). Set it to zero for no limitation. If hardware \n"
             "acceleration is enabled, this parameter is set to the number of the accelerator queues.")
      ->capture_default_str()
      ->check(CLI::Number);
  add_option(app,
             "--max_pdsch_concurrency",
             config.max_pdsch_concurrency,
             "Maximum concurrency level for PDSCH processing for all cells.\n"
             "Limits the number of threads that can concurrently process Physical Downlink Shared Channel (PDSCH).\n"
             "Set to zero for no limitation. If hardware acceleration is enabled, this parameter is set to the\n"
             "number of the accelerator queues.")
      ->capture_default_str()
      ->check(CLI::Number);
}

static void configure_cli11_expert_execution_args(CLI::App& app, du_low_unit_expert_execution_config& config)
{
  // Threads section.
  CLI::App* threads_subcmd = add_subcommand(app, "threads", "Threads configuration")->configurable();

  // Upper PHY threads.
  CLI::App* upper_phy_threads_subcmd =
      add_subcommand(*threads_subcmd, "upper_phy", "Upper PHY thread configuration")->configurable();
  configure_cli11_upper_phy_threads_args(*upper_phy_threads_subcmd, config.threads);
}

static void configure_cli11_expert_phy_args(CLI::App& app, du_low_unit_expert_upper_phy_config& expert_phy_params)
{
  auto pusch_sinr_method_check = [](const std::string& value) -> std::string {
    if ((value == "channel_estimator") || (value == "post_equalization") || (value == "evm")) {
      return {};
    }
    return "Invalid PUSCH SINR calculation method. Accepted values [channel_estimator,post_equalization,evm]";
  };
  auto pusch_channel_estimator_fd_strategy_method_check = [](const std::string& value) -> std::string {
    if ((value == "filter") || (value == "mean") || (value == "none")) {
      return {};
    }
    return "Invalid PUSCH channel estimator frequency-domain strategy. Accepted values [filter,mean,none]";
  };
  auto pusch_channel_estimator_td_strategy_method_check = [](const std::string& value) -> std::string {
    if ((value == "average") || (value == "interpolate")) {
      return {};
    }
    return "Invalid PUSCH channel estimator time-domain strategy. Accepted values [average,interpolate]";
  };
  auto pusch_channel_equalizer_algorithm_method_check = [](const std::string& value) -> std::string {
    if ((value == "zf") || (value == "mmse")) {
      return {};
    }
    return "Invalid PUSCH channel equalizer algorithm. Accepted values [zf,mmse]";
  };

  add_option(app,
             "--max_proc_delay",
             expert_phy_params.max_processing_delay_slots,
             "Maximum allowed DL processing delay in slots.")
      ->capture_default_str()
      ->check(CLI::Range(1, 30));
  add_option(app,
             "--pusch_dec_max_iterations",
             expert_phy_params.pusch_decoder_max_iterations,
             "Maximum number of PUSCH LDPC decoder iterations")
      ->capture_default_str()
      ->check(CLI::Number);
  add_option(app,
             "--pusch_dec_enable_early_stop",
             expert_phy_params.pusch_decoder_early_stop,
             "Enables PUSCH LDPC decoder early stop")
      ->capture_default_str();
  add_option(app,
             "--pusch_decoder_force_decoding",
             expert_phy_params.pusch_decoder_force_decoding,
             "Forces PUSCH LDPC decoder to decode always")
      ->capture_default_str();
  add_option(app,
             "--pusch_sinr_calc_method",
             expert_phy_params.pusch_sinr_calc_method,
             "PUSCH SINR calculation method: channel_estimator, post_equalization and evm.")
      ->capture_default_str()
      ->check(pusch_sinr_method_check);
  add_option(app,
             "--pusch_channel_estimator_fd_strategy",
             expert_phy_params.pusch_channel_estimator_fd_strategy,
             "PUSCH channel estimator frequency-domain smoothing strategy: filter, mean and none.")
      ->capture_default_str()
      ->check(pusch_channel_estimator_fd_strategy_method_check);
  add_option(app,
             "--pusch_channel_estimator_td_strategy",
             expert_phy_params.pusch_channel_estimator_td_strategy,
             "PUSCH channel estimator time-domain strategy: average and interpolate.")
      ->capture_default_str()
      ->check(pusch_channel_estimator_td_strategy_method_check);
  add_option(app,
             "--pusch_channel_estimator_cfo_compensation",
             expert_phy_params.pusch_channel_estimator_cfo_compensation,
             "PUSCH channel estimator CFO compensation.")
      ->capture_default_str();
  add_option(app,
             "--pusch_channel_equalizer_algorithm",
             expert_phy_params.pusch_channel_equalizer_algorithm,
             "PUSCH channel equalizer algorithm: zf and mmse.")
      ->capture_default_str()
      ->check(pusch_channel_equalizer_algorithm_method_check);
  add_option(app,
             "--max_request_headroom_slots",
             expert_phy_params.nof_slots_request_headroom,
             "Maximum request headroom size in slots.")
      ->capture_default_str()
      ->check(CLI::Range(0, 30));
  add_option(app,
             "--allow_request_on_empty_uplink_slot",
             expert_phy_params.allow_request_on_empty_uplink_slot,
             "Generates an uplink request in an uplink slot with no PUCCH/PUSCH/SRS PDUs")
      ->capture_default_str();
  add_option(app,
             "--enable_phy_tap",
             expert_phy_params.enable_phy_tap,
             "Enables or disables the PHY tap plugin if it is present while building the application.")
      ->capture_default_str();
  add_option(app,
             "--phy_tap_arguments",
             expert_phy_params.phy_tap_arguments,
             "PHY tap plugin argument string passed during construction.")
      ->capture_default_str();
}

#ifdef DPDK_FOUND
static void configure_cli11_hwacc_pdsch_enc_args(CLI::App& app, std::optional<hwacc_pdsch_appconfig>& config)
{
  config.emplace();

  app.add_option("--nof_hwacc", config->nof_hwacc, "Number of hardware-accelerated PDSCH encoding functions")
      ->capture_default_str()
      ->check(CLI::Range(0, 64));
  app.add_option("--cb_mode", config->cb_mode, "Operation mode of the PDSCH encoder (CB = true, TB = false [default])")
      ->capture_default_str();
  app.add_option("--max_buffer_size",
                 config->max_buffer_size,
                 "Maximum supported buffer size in bytes (CB mode will be forced for larger TBs)")
      ->capture_default_str();
  app.add_option("--dedicated_queue",
                 config->dedicated_queue,
                 "Hardware queue use for the PDSCH encoder (dedicated = true [default], shared = false)")
      ->capture_default_str();
}
static void configure_cli11_hwacc_pusch_dec_args(CLI::App& app, std::optional<hwacc_pusch_appconfig>& config)
{
  config.emplace();

  app.add_option("--nof_hwacc", config->nof_hwacc, "Number of hardware-accelerated PDSCH encoding functions")
      ->capture_default_str()
      ->check(CLI::Range(0, 64));
  app.add_option("--harq_context_size", config->harq_context_size, "Size of the HARQ context repository")
      ->capture_default_str();
  app.add_option(
         "--force_local_harq", config->force_local_harq, "Force using the host memory to implement the HARQ buffer")
      ->capture_default_str();
  app.add_option("--dedicated_queue",
                 config->dedicated_queue,
                 "Hardware queue use for the PUSCH decoder (dedicated = true [default], shared = false)")
      ->capture_default_str();
}

static void configure_cli11_bbdev_hwacc_args(CLI::App& app, std::optional<bbdev_appconfig>& config)
{
  auto hwacc_type_check = [](const std::string& value) -> std::string {
    if ((value == "acc100") || (value == "acc200") || (value == "vrb1")) {
      return {};
    }
    return "Invalid BBDEV hardware-accelerator type. Accepted values [acc100,acc200,vrb1]";
  };

  config.emplace();

  app.add_option("--hwacc_type", config->hwacc_type, "Type of BBDEV hardware-accelerator")
      ->capture_default_str()
      ->check(hwacc_type_check);
  app.add_option("--id", config->id, "ID of the BBDEV-based hardware-accelerator.")
      ->capture_default_str()
      ->check(CLI::Range(0, 65535));

  // (Optional) Hardware-accelerated PDSCH encoding functions configuration.
  CLI::App* hwacc_pdsch_enc_subcmd =
      app.add_subcommand("pdsch_enc", "Hardware-accelerated PDSCH encoding functions configuration");
  configure_cli11_hwacc_pdsch_enc_args(*hwacc_pdsch_enc_subcmd, config->pdsch_enc);

  // (Optional) Hardware-accelerated PUSCH decoding functions configuration.
  CLI::App* hwacc_pusch_dec_subcmd =
      app.add_subcommand("pusch_dec", "Hardware-accelerated PUSCH decoding functions configuration");
  configure_cli11_hwacc_pusch_dec_args(*hwacc_pusch_dec_subcmd, config->pusch_dec);

  app.add_option("--msg_mbuf_size",
                 config->msg_mbuf_size,
                 "Size of the mbufs storing unencoded and unrate-matched messages (in bytes)")
      ->capture_default_str()
      ->check(CLI::Range(0, 64000));
  app.add_option("--rm_mbuf_size",
                 config->rm_mbuf_size,
                 "Size of the mbufs storing encoded and rate-matched messages (in bytes)")
      ->capture_default_str()
      ->check(CLI::Range(0, 64000));
  app.add_option("--nof_mbuf", config->nof_mbuf, "Number of mbufs in the memory pool")->capture_default_str();
}

static void configure_cli11_hal_args(CLI::App& app, std::optional<du_low_unit_hal_config>& config)
{
  config.emplace();

  // (Optional) BBDEV-based hardware-accelerator configuration.
  CLI::App* bbdev_hwacc_subcmd =
      add_subcommand(app, "bbdev_hwacc", "BBDEV-based hardware-acceleration configuration parameters");
  configure_cli11_bbdev_hwacc_args(*bbdev_hwacc_subcmd, config->bbdev_hwacc);
}
#endif

#ifdef DPDK_FOUND
static void manage_hal_optional(CLI::App& app, du_low_unit_config& parsed_cfg)
{
  // Clean the HAL optional.
  if (app.get_subcommand("hal")->count_all() == 0) {
    parsed_cfg.hal_config.reset();

    return;
  }

  const auto& hal = app.get_subcommand("hal");
  if (hal->get_subcommand("bbdev_hwacc")->count_all() == 0) {
    parsed_cfg.hal_config->bbdev_hwacc.reset();
  }
}
#endif

static void configure_cli11_metrics_args(CLI::App& app, du_low_unit_metrics_config& metrics_params)
{
  CLI::App* layers_subcmd = add_subcommand(app, "layers", "Layer basis metrics configuration")->configurable();
  add_option(
      *layers_subcmd, "--enable_du_low", metrics_params.enable_du_low, "Enable DU low metrics (upper physical layer)")
      ->capture_default_str();
}

void srsran::configure_cli11_with_du_low_config_schema(CLI::App& app, du_low_unit_config& parsed_cfg)
{
  // Loggers section.
  CLI::App* log_subcmd = add_subcommand(app, "log", "Logging configuration")->configurable();
  configure_cli11_log_args(*log_subcmd, parsed_cfg.loggers);

  // Tracer section.
  CLI::App* trace_subcmd = add_subcommand(app, "trace", "General tracer configuration")->configurable();
  configure_cli11_trace_args(*trace_subcmd, parsed_cfg.tracer);

  // Expert upper PHY section.
  CLI::App* expert_phy_subcmd =
      add_subcommand(app, "expert_phy", "Expert physical layer configuration")->configurable();
  configure_cli11_expert_phy_args(*expert_phy_subcmd, parsed_cfg.expert_phy_cfg);

  // Expert execution section.
  CLI::App* expert_subcmd = add_subcommand(app, "expert_execution", "Expert execution configuration")->configurable();
  configure_cli11_expert_execution_args(*expert_subcmd, parsed_cfg.expert_execution_cfg);

#ifdef DPDK_FOUND
  // HAL section.
  CLI::App* hal_subcmd = add_subcommand(app, "hal", "HAL configuration")->configurable();
  configure_cli11_hal_args(*hal_subcmd, parsed_cfg.hal_config);
#endif

  // Metrics section.
  app_helpers::configure_cli11_with_metrics_appconfig_schema(app, parsed_cfg.metrics_cfg.common_metrics_cfg);
  CLI::App* metrics_subcmd = add_subcommand(app, "metrics", "Metrics configuration")->configurable();
  configure_cli11_metrics_args(*metrics_subcmd, parsed_cfg.metrics_cfg);
}

void srsran::autoderive_du_low_parameters_after_parsing(CLI::App& app, du_low_unit_config& parsed_cfg, duplex_mode mode)
{
  // If max proc delay property is not present in the config, configure the default value.
  CLI::App* expert_cmd = app.get_subcommand("expert_phy");
  if (expert_cmd->count_all() == 0 || expert_cmd->count("--max_proc_delay") == 0) {
    switch (mode) {
      case duplex_mode::TDD:
        parsed_cfg.expert_phy_cfg.max_processing_delay_slots = 5;
        break;
      case duplex_mode::FDD:
        parsed_cfg.expert_phy_cfg.max_processing_delay_slots = 2;
        break;
      default:
        break;
    }
  }

  // If max request headroom slots property is present in the config, do nothing.
  if (expert_cmd->count_all() == 0 || expert_cmd->count("--max_request_headroom_slots") == 0) {
    parsed_cfg.expert_phy_cfg.nof_slots_request_headroom = parsed_cfg.expert_phy_cfg.max_processing_delay_slots;
  }

#ifdef DPDK_FOUND
  manage_hal_optional(app, parsed_cfg);
#endif
}
