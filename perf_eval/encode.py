"""
Number of bits to use in the binary format after encoding, depends on the data type used
in the kernel. We are using 32 bit int, so stick to 32 bits otherwise kstrtoint() will fail.

NOTE: BINARY IS NOT USED ANYMORE. INTEGERS ARE USED DIRECTLY.

"""
BIN_FORMAT = '032b'

def encode_data(user_attr_map, obj_attr_map, env_attr_map, current_env_attrs, policy):
    # Encode abac data into binary
    strs = []
    for uid, attrs in user_attr_map.items():
        for name, value in attrs.items():
            strs.append(name)
            strs.append(value)
    for path, attrs in obj_attr_map.items():
        for name, value in attrs.items():
            strs.append(name)
            strs.append(value)
    for e, attrs in env_attr_map.items():
        for name, value in attrs.items():
            strs.append(name)
            strs.append(value)
    for rule in policy:
        for name, value in rule["user"].items():
            strs.append(name)
            strs.append(value)
        for name, value in rule["obj"].items():
            strs.append(name)
            strs.append(value)
        for name, value in rule["env"].items():
            strs.append(name)
            strs.append(value)

    strs = list(set(strs))
    encoding = {}
    for i, s in enumerate(strs):
        #encoding[s] = format(i, BIN_FORMAT)
        encoding[s] = i + 1

    user = {}
    obj = {}
    env = {}
    ce_map = {}
    _policy = []

    for uid, attrs in user_attr_map.items():
        user[uid] = {}
        for name, value in attrs.items():
            user[uid][encoding[name]] = encoding[value]

    for path, attrs in obj_attr_map.items():
        obj[path] = {}
        for name, value in attrs.items():
            obj[path][encoding[name]] = encoding[value]

    for e, attrs in env_attr_map.items():
        env[e] = {}
        for name, value in attrs.items():
            env[e][encoding[name]] = encoding[value]

    for name, value in current_env_attrs.items():
        ce_map[encoding[name]] = encoding[value]

    for rule in policy:
        n = {"user": {}, "obj":{}, "env":{}, "id": rule["id"], "op": rule["op"]}
        for name, value in rule["user"].items():
            n["user"][encoding[name]] = encoding[value]
        for name, value in rule["obj"].items():
            n["obj"][encoding[name]] = encoding[value]
        for name, value in rule["env"].items():
            n["env"][encoding[name]] = encoding[value]
        _policy.append(n)

    return user, obj, env, ce_map, _policy
