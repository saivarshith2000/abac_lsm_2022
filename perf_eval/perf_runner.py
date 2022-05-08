import sys
import json
from pathlib import Path
from time import localtime, strftime
from perf import main as perf

def main(kernel_model, dac_only=False):
    Path(f"./results/").mkdir(parents=True, exist_ok=True)
    results = {}
    timestamp = strftime("%Y-%m-%d %I:%M:%S %p", localtime())
    results['timestamp'] = timestamp
    results['kernel_model'] = kernel_model
    results['abac'] = {}
    results['dac_only'] = {}

    with open('./config/data_config_map.json') as f:
        data_config_map = json.load(f)

    for config_path, data_path in data_config_map.items():
        if dac_only:
            print(f"[DAC_ONLY] Running performance analysis for {config_path}")
        else:
            print(f"[WITH ABAC] Running performance analysis for {config_path}")

        stats = perf(config_path, data_path, kernel_model, dac_only)

        if dac_only:
            results['dac_only'][stats['config']] = stats
        else:
            results['abac'][stats['config']] = stats

        with open(f'./results/{kernel_model}.json', 'w') as f:
            json.dump(results, f, indent=2)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(f"Invalid Usage\npython3 {sys.argv[0]} <kernel_model>\
                \nKernel model is the ABAC lsm that is currently loaded. Must be one of abac_trees, abac_trees_enc, abac_rules, abac_rules_enc")
    if len(sys.argv) == 3:
        main(sys.argv[1], dac_only=True)
    else:
        main(sys.argv[1])


