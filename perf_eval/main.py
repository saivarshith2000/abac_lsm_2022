# Entry point for data generation
# generates individual configs based on the main configs in 'config/' directory
# These generated configs are used to create datasets in 'data/' directory
from generate_configs import main as generate_configs
from generate_raw import main as generate_raw
from generate_rule_abacfs import main as generate_rule_abacfs
from generate_tree_abacfs import main as generate_tree_abacfs
from multiprocessing import Pool

def main():
    configs = generate_configs()
    for c in configs:
        outfile, config_name = generate_raw(c)
        generate_rule_abacfs(outfile)
        generate_tree_abacfs(outfile)

if __name__ == "__main__":
    main()

