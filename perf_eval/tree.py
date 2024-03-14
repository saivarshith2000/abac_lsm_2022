import random
import math
from anytree import AnyNode, RenderTree
from pprint import pprint

class AttrNode:
    def __init__(self, nid, attr=None, op=None):
        self.attr = attr
        self.op = op
        self.nid = nid
        self.children = {}
        self.parent = None

    def add_child(self, value, child):
        if child.attr == self.attr or child.attr in self.children.keys():
            raise Exception(f"attribute {child.attr} already exists")
        self.children[value] = child

def get_possible_values(a, rules):
    vals = []
    for r in rules:
        if a in r['user']:
            vals.append(r['user'][a])
        elif a in r['env']:
            vals.append(r['env'][a])
    return list(set(vals))

def filter_rules(a, v, rules):
    filtered = []
    for r in rules:
        if a in r['user'] and r['user'][a] == v:
            filtered.append(r)
        elif a in r['env'] and r['env'][a] == v:
            filtered.append(r)
    return filtered

def max_entropy_attr(attributes, rules, umap, emap):
    """
    Get the attribute with maximum entropy given mappings of user and environmental attributes
    And also return its possible values (reduces extra get call to get_possible_values)
    """
    max_entropy = float('-inf')
    max_attr = None
    max_vals = None
    for a in attributes:
        vals = get_possible_values(a, rules)

        #if a does not appear in rules, skip it
        if len(vals) == 0:
            continue

        entropy = 0
        is_user = False
        for v in vals:
            prob = 0
            # check user entities
            for u, avp in umap.items():
                if v in avp.values():
                    prob += 1
                    is_user = True

            # check env entities
            if not is_user:
                for u, avp in emap.items():
                    if v in avp.values():
                        prob += 1
            # calculate probability and entropy
            if is_user:
                prob /= len(umap.keys())
            else:
                prob /= len(emap.keys())
            if prob != 0:
                entropy += -1 * (prob * math.log(prob))

        if entropy > max_entropy:
            max_entropy = entropy
            max_attr = a
            max_vals = vals
    #print(f"{max_entropy}\n{max_vals}")
    return max_attr, max_vals

def build_attr_tree(attributes, rules, umap, emap):
    return build_attr_tree_r(attributes, rules, umap, emap, [0])

def build_attr_tree_r(attributes, rules, umap, emap, nid):
    """
    Recursive method to build attribute tree using user and environmental attributes, and policy
    """
    if len(attributes) == 0:
        return AttrNode(nid[0], op = rules[0]["op"])
    # Get the attribute with maximum entropy and its possible values
    a, vals = max_entropy_attr(attributes, rules, umap, emap)

    # If no attribute appears in rules
    if vals == None:
        return AttrNode(nid[0], op = rules[0]["op"])
    
    n = AttrNode(nid[0], attr = a)
    for v in vals:
        child_rules = filter_rules(a, v, rules)
        child_attributes = [x for x in attributes if x != a]
        nid[0] += 1
        child = build_attr_tree_r(child_attributes, child_rules, umap, emap, nid)
        n.add_child(v, child)
        child.parent = n
    return n

def count_nodes(tree):
    """
    Recursive method to count number of nodes in the tree
    """
    if tree.attr is None:
        return 1
    count = 1
    for child in tree.children.values():
        count += count_nodes(child)
    return count

def serialize_r(root, value = None):
    """
    Recursive helper function for serializing the tree
    """
    retval = f"{root.nid} "
    if root.parent:
        retval += f"{root.parent.nid} "
    else:
        retval += "- "
    if value:
        retval += f"{value} "
    else:
        retval += "- "
    if not root.attr:
        retval += f"{root.op}"
    else:
        retval += f"{root.attr}"
    retval += "|"
    for val, child in root.children.items():
        retval += serialize_r(child, val)
    return retval

def serialize(tree):
    """
    Returns the string representation of a tree in the following format

    n
    nid1 parent-id1 value_of_parent1 attribute1
    nid2 parent-id2 value_of_parent2 attribute2
    ...

    The first line contains the number of nodes in the tree.
    Apart from the first line each line represents the details of a single node.
    """
    n = count_nodes(tree)
    return f"{n}|" + serialize_r(tree)[:-1]

def parse_node(line):
    """
    Parses a single non-root node section in the serialized representation
    """
    nid, pid, value, lastval = line.split()
    if lastval in ["MODIFY", "READ"]:
        return int(pid), value, AttrNode(int(nid), op=lastval)
    else:
        return int(pid), value, AttrNode(int(nid), attr=lastval)

def deserialize(tree_str):
    """De-serialize the serialized representation of the attr tree"""
    lines = tree_str.split("|")
    n = int(lines[0])
    #print(f"Tree has {n} nodes...")
    nodes = [None] * n
    root = AttrNode(0, attr = lines[1].split()[-1])
    nodes[0] = root
    for line in lines[2:]:
        pid, value, node = parse_node(line)
        #print(node.nid, pid, value, node)
        nodes[node.nid] = node
        nodes[pid].add_child(value, node)
    return root

def get_render_tree(root, parent = None, val = None):
    # build a tree using the anytree library and print it
    if root is None:
        return
    if parent is None:
        # root node
        r = AnyNode(nid=root.nid, attr=root.attr, val=val)
    elif root.attr is None:
        # leaf node
        r = AnyNode(nid=root.nid, attr=root.op, parent=parent, val=val)
    else:
        # intermediate nodes
        r = AnyNode(nid=root.nid, attr=root.attr, parent=parent, val=val)
    for value, child in root.children.items():
        get_render_tree(child, parent=r, val=value)
    return r

def print_tree(root):
    print(RenderTree(get_render_tree(root)))
