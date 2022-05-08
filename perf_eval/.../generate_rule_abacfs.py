import sys
import json
from pprint import pprint
from encode import encode_data
from pathlib import Path

"""
Does the following things
1. Build object specific rule lists, map them to corresponding object paths and serialize the mapping
2. Serialize user attributes and current environmental attributes
3. Writes serialized data to 'data/original' directory
4. Repeats steps 1-2 but encodes the data and saves them in 'data/encoded' directory
"""

not_found = 0
found = 0
more_than_one = 0

def get_covering_rules(obj_attrs, policy):
    """
    Get rules in @policy that cover an object with attributes @obj_attrs
    """
    r = []
    for rule in policy:
        covering = True
        # Each attribute of the object must have a matching value
        # Otherwise the rule is not covering the object
        for attr, value in obj_attrs.items():
            if attr not in rule["obj"] or rule["obj"][attr] != value:
                covering = False
                break
        if rule in r or not covering:
            continue
        r.append(str(rule['id']))
    global found, not_found, more_than_one
    if len(r) == 0:
        not_found += 1
    elif len(r) > 1:
        more_than_one += 1
    else:
        found += 1
    return r

def build_rule_map(obj_attr_map, policy):
    """
    Build a dictionary of object paths mapped to serialized version of their rule set
    """
    rule_map = {}
    for path, obj_attrs in obj_attr_map.items():
        rules = get_covering_rules(obj_attrs, policy)
        rule_str = ",".join(rules)
        rule_map[path] = rule_str
    return rule_map

def save(rule_map, user_attr_map, current_env_attrs, policy, config_name, enc=False):
    directory = f'data/{config_name}/rules/original'
    if enc:
        directory = f'data/{config_name}/rules/encoded'
    # save Rule list mapping
    data = ""
    for path, rule_list in rule_map.items():
        if len(rule_list) != 0:
            data += f"{path}:{rule_list}\n"
    data = data[:-1]
    with open(f'{directory}/obj_rules', 'w') as f:
        f.write(data)

    # save user attribute mapping
    data = ""
    for uid, attrs in user_attr_map.items():
        data += f"{uid}:"
        for name, value in attrs.items():
            data += f"{name}={value},"
        data = data[:-1] + "\n"
    with open(f'{directory}/user_attr', 'w') as f:
        f.write(data)

    # save current environmental attribute values
    data = ""
    for name, value in current_env_attrs.items():
        data += f"{name}={value}\n"
    data = data[:-1]
    with open(f'{directory}/env_attr', 'w') as f:
        f.write(data)

    # save policy
    data = f"{len(policy)}\n"
    for rule in policy:
        # Write user, env attrs and operation
        data += f"{rule['id']}:"
        for name, value in rule['user'].items():
            data += f"{name}={value},"
        data = data[:-1] + '|'
        for name, value in rule['env'].items():
            data += f"{name}={value},"
        data = data[:-1] + '|'
        data += rule['op']
        data += '\n'
    with open(f'{directory}/policy', 'w') as f:
        f.write(data)


def main(data_fname):
    global found, not_found, more_than_one
    found = 0
    not_found = 0
    more_than_one = 0
    with open(data_fname) as f:
        data = json.load(f)
        print(f"Loaded data from {data_fname}")
    config_name = data['config']

    # Create necessary directories
    Path(f"./data/{config_name}/rules/original/").mkdir(parents=True, exist_ok=True)
    Path(f"./data/{config_name}/rules/encoded/").mkdir(parents=True, exist_ok=True)
    print(f"Directories 'data/{config_name}/rules/original' and 'data/{config_name}/rules/encoded' created")

    # Build tree using original data
    print("Building rule map...")
    rule_map = build_rule_map(data['obj'], data['policy'])
    save(rule_map, data['user'], data['current_env'], data['policy'], config_name)
    print(f"Data written to data/{config_name}/rules/original/\n")

    # Encode data
    print("Encoding data...")
    umap, omap, emap, ce_attrs, _policy = encode_data(data['user'], data['obj'], data['env'], data['current_env'], data['policy'])
    print("Building rule map...")
    rule_map = build_rule_map(omap, _policy)
    save(rule_map, umap, ce_attrs, _policy, config_name, enc=True)
    print(f"Data written to data/{config_name}/rules/encoded/\n")
    print(f"Objects with rules: {int(found/2)}")
    print(f"Objects with more than one rule: {int(more_than_one/2)}")
    print(f"Objects without rules: {int(not_found/2)}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Invalid usage\npython3 {sys.argv[0]} <data_json>")
        sys.exit(-1)
    main(sys.argv[1])
