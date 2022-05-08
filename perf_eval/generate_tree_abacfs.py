import sys
import json
from pprint import pprint
from tree import build_attr_tree, serialize
from encode import encode_data
from pathlib import Path

"""
Does the following things
1. Build object specific NPolTrees, map them to corresponding object paths and serialize the mapping
2. Serialize user attributes and current environmental attributes
3. Writes serialized data to 'data/original' directory
4. Repeats steps 1-2 but encodes the data and saves them in 'data/encoded' directory
"""

def get_covering_rules(obj_attrs, policy):
    """
    Get rules in @policy that cover an object with attributes @obj_attrs
    """
    r = []
    for rule in policy:
        covering = True
        for attr, value in obj_attrs.items():
            if attr not in rule["obj"]:
                covering = False
                break
            if rule["obj"][attr] != value:
                covering = False
                break
        if rule in r or not covering:
            continue
        r.append(rule)
    return r

def build_tree_map(user_attr_map, obj_attr_map, env_attr_map, policy):
    """
    Build a dictionary of object paths mapped to serialized version of their NPolTrees
    """
    tree_map = {}
    for path, obj_attrs in obj_attr_map.items():
        print(f"Object: {path}")
        rules = get_covering_rules(obj_attrs, policy)
        if len(rules) == 0:
            print("no rules found")
        attributes = []
        # user attributes
        for r in rules:
            for attr, value in r["user"].items():
                if attr not in attributes:
                    attributes.append(attr)
        # env attributes
        for r in rules:
            for attr, value in r["env"].items():
                if attr not in attributes:
                    attributes.append(attr)
        tree = build_attr_tree(attributes, rules, user_attr_map, env_attr_map)
        tree_map[path] = tree
    return tree_map

def save(trees, user_attr_map, current_env_attrs, config_name, enc=False):
    directory = f'data/{config_name}/trees/original'
    if enc:
        directory = f'data/{config_name}/trees/encoded'
    # save NPoltree mapping
    data = ""
    for path, tree in trees.items():
        tree_str = serialize(tree).strip()
        data += f"{path}:{tree_str}\n"
    data = data[:-1]
    with open(f'{directory}/obj_attr', 'w') as f:
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

def main(data_fname):
    with open(data_fname) as f:
        data = json.load(f)
        print(f"Loaded data from {data_fname}")
    config_name = data['config']

    # Create necessary directories
    Path(f"./data/{config_name}/trees/original/").mkdir(parents=True, exist_ok=True)
    Path(f"./data/{config_name}/trees/encoded/").mkdir(parents=True, exist_ok=True)
    print(f"Directories 'data/{config_name}/trees/original' and 'data/{config_name}/trees/encoded' created")

    # Build tree using original data
    print("Building trees...")
    tree_map = build_tree_map(data['user'], data['obj'], data['env'], data['policy'])
    save(tree_map, data['user'], data['current_env'], config_name)
    print(f"Data written to data/{config_name}/trees/original/\n")

    # Encode data
    print("Encoding data...")
    umap, omap, emap, ce_attrs, _policy = encode_data(data['user'], data['obj'], data['env'], data['current_env'], data['policy'])
    print("Building trees...")
    tree_map = build_tree_map(umap, omap, emap, _policy)
    save(tree_map, umap, ce_attrs, config_name, enc=True)
    print(f"Data written to data/{config_name}/trees/encoded/\n")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Invalid usage\npython3 {sys.argv[0]} <data_json>")
        sys.exit(-1)
    main(sys.argv[1])
