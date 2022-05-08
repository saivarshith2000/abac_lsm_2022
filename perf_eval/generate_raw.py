import sys
import json
import random
import pprint
from pathlib import Path

def generate_policy(config):
    # Generate ABAC policy based on values given in config
    # Returns the policy, possible values for each user and object attribute
    policy = []
    min_user_attrs = int(config['user']['attrs'] * 0.25)
    min_obj_attrs = int(config['obj']['attrs'] * 0.25)
    # min_user_attrs = 1
    # min_obj_attrs = 1

    for i in range(config['policy']['count']):
        rule = {'id': i, 'user': {}, 'obj': {}, 'env': {}, 'op': random.choice(["MODIFY", "READ"])}
        for j in range(random.randrange(min_user_attrs, config['user']['attrs'])):
            attr = f"ua_{j}_v_{random.choice(range(config['user']['values_per_attr']))}" 
            rule['user'][f'ua_{j}'] = attr

        for j in range(random.randrange(min_obj_attrs, config['obj']['attrs'])):
            attr = f"oa_{j}_v_{random.choice(range(config['obj']['values_per_attr']))}" 
            rule['obj'][f'oa_{j}'] = attr

        for j in range(config['env']['attrs']):
            attr = f"ea_{j}_v_{random.choice(range(config['env']['values_per_attr']))}" 
            rule['env'][f'ea_{j}'] = attr

        policy.append(rule)

    return policy

def generate_mapping(config, policy):
    # Generate attribute mapping to users, objects and environmental states
    mapping = {}
    mapping['user'] = {}
    mapping['obj'] = {}
    mapping['env'] = {}
    mapping['current_env'] = {}

    # Map environmental entities
    j = 0
    for i in range(config['policy']['count']):
        if policy[i]['env'] not in mapping['env'].values():
            mapping['env'][f'e_{j}'] = policy[i]['env']
            j += 1

    policy_idx = list(range(config['policy']['count']))
    random.shuffle(policy_idx)

    # Map user and object entities
    if config['user']['count'] <= config['policy']['count']:
        for i in range(config['user']['count']):
            mapping['user'][f'{1000 + i}'] = policy[policy_idx[i]]['user']
    else:
        ratio = int(config['user']['count']/config['policy']['count'])
        u_count = 0
        for i in range(config['policy']['count']):
            for j in range(ratio):
                mapping['user'][f'{1000 + u_count}'] = policy[policy_idx[i]]['user']
                u_count += 1
        if u_count != config['user']['count']:
            diff = config['user']['count'] - u_count
            for i in range(diff):
                mapping['user'][f'{1000 + u_count}'] = policy[policy_idx[i]]['user']
                u_count += 1
        print(f"user {u_count} = {config['user']['count']}")

    if config['obj']['count'] <= config['policy']['count']:
        for i in range(config['obj']['count']):
            mapping['obj'][f'/home/secured/obj_{i}'] = policy[policy_idx[i]]['obj']
    else:
        ratio = int(config['obj']['count']/config['policy']['count'])
        o_count = 0
        for i in range(config['policy']['count']):
            for j in range(ratio):
                mapping['obj'][f'/home/secured/obj_{o_count}'] = policy[policy_idx[i]]['obj']
                o_count += 1
        if o_count != config['obj']['count']:
            diff = config['obj']['count'] - o_count
            for i in range(diff):
                mapping['obj'][f'/home/secured/obj_{o_count}'] = policy[policy_idx[i]]['obj']
                o_count += 1
        print(f"obj {o_count} = {config['obj']['count']}")

    # randomly remove attributes to improve chance of having more than one rule
    for i in range(config['user']['count']):
        assigned_attrs = len(mapping['user'][f'{1000 + i}'].keys()) 
        if assigned_attrs > 1:
            num_attrs = random.choice(range(1, assigned_attrs))
        else:
            num_attrs = 1
        remaining_attrs = random.sample(list(mapping['user'][f'{1000 + i}'].keys()), num_attrs)
        avps = {}
        for attr in remaining_attrs:
            avps[attr] = mapping['user'][f'{1000 + i}'][attr]
        mapping['user'][f'{1000 + i}'] = avps

    for i in range(config['obj']['count']):
        path = f'/home/secured/obj_{i}'
        assigned_attrs = len(mapping['obj'][path].keys())
        if assigned_attrs > 1:
            num_attrs = random.choice(range(1, assigned_attrs))
        else:
            num_attrs = 1
        remaining_attrs = random.sample(list(mapping['obj'][path].keys()), num_attrs)
        avps = {}
        for attr in remaining_attrs:
            avps[attr] = mapping['obj'][path][attr]
        mapping['obj'][path] = avps

    # Map current environmental attributes
    #mapping['current_env'] = mapping['env'][random.choice(list(mapping['env'].keys()))]
    mapping['current_env'] = mapping['env']['e_0']

    return mapping

def main(fname):
    with open(fname) as f:
        config_str = f.read()
    config = json.loads(config_str)
    print(f"Config {config['name']} loaded from {fname}")
    #print(config_str)
    policy = generate_policy(config)
    mapping = generate_mapping(config, policy)
    data = {}
    data['config'] = config['name']
    data['user'] = mapping['user']
    data['obj'] = mapping['obj']
    data['env'] = mapping['env']
    data['current_env'] = mapping['current_env']
    data['policy'] = policy
    # pprint.pprint(data, sort_dicts=False)

    Path(f"./data/{config['name']}/").mkdir(parents=True, exist_ok=True)
    outfile = f"./data/{config['name']}/raw.json"
    with open(outfile, 'w') as f:
        json.dump(data, f, indent=2)
        print(f"data written to {outfile}")
    return outfile, config['name']

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Invalid usage\npython3 {sys.argv[0]} <config_path>")
        sys.exit(-1)
    main(sys.argv[1])
