import json
import re
from glob import glob
from pathlib import Path

def main():
    # Create path for individual configs
    Path(f"./config/generated/").mkdir(parents=True, exist_ok=True)
    fnames = list(glob('./config/*.json'))
    fnames = [x for x in fnames if x != "./config/data_config_map.json"]
    fnames.sort()
    generated_configs = []
    data_config_map = {}
    for fname in fnames:
        print(f"\nReading {fname}")
        varying = None
        with open(fname) as f:
            main_config = json.load(f)
        if re.match(r"user_attributes_*", main_config['name']):
            # varying number of user attributes
            for c in main_config['user']['attrs']:
                config = main_config.copy()
                config['user']['attrs'] = c
                config_name = f"{config['name']}_{c}"
                config['name'] = config_name
                outfile = f"./config/generated/{config_name}.json"
                data_path = f"./data/{config_name}"
                with open(outfile, 'w') as f:
                    json.dump(config, f, indent=4)
                print(f"Wrote {outfile}")
                generated_configs.append(outfile)
                data_config_map[outfile] = data_path
            continue

        elif re.match(r"values_per_user_attributes*", main_config['name']):
            # varying number of values per user attribute
            for c in main_config['user']['values_per_attr']:
                config = main_config.copy()
                config['user']['values_per_attr'] = c
                config_name = f"{config['name']}_{c}"
                config['name'] = config_name
                outfile = f"./config/generated/{config_name}.json"
                data_path = f"./data/{config_name}"
                with open(outfile, 'w') as f:
                    json.dump(config, f, indent=4)
                print(f"Wrote {outfile}")
                generated_configs.append(outfile)
                data_config_map[outfile] = data_path
            continue

        elif re.match(r"user_*", main_config['name']):
            # varying number of users
            varying = 'user'
        elif re.match(r"objects_*", main_config['name']):
            # varying number of objects
            varying = 'obj'
        elif re.match(r"rules_*", main_config['name']):
            # varying number of rules
            varying = 'policy'
        for c in main_config[varying]['count']:
            config = main_config.copy()
            config[varying]['count'] = c
            config_name = f"{config['name']}_{c}"
            config['name'] = config_name
            outfile = f"./config/generated/{config_name}.json"
            data_path = f"./data/{config_name}"
            with open(outfile, 'w') as f:
                json.dump(config, f, indent=4)
            print(f"Wrote {outfile}")
            generated_configs.append(outfile)
            data_config_map[outfile] = data_path

    with open('./config/data_config_map.json', 'w') as f:
        json.dump(data_config_map, f, indent=2)

    print("\n---------------------------------------------------")
    print(f"Read {len(fnames)} main configs")
    print(f"Wrote {len(generated_configs)} configs in config/generated/")
    print("Data-Config mapping written to './config/data_config_map.json'")
    print("---------------------------------------------------")
    generated_configs.sort()
    return generated_configs

if __name__ == "__main__":
    main()
