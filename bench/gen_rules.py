#!/usr/bin/env python3
"""Generate JSON rule files of varying sizes for benchmarking."""
import json
import os
import argparse

def gen_rules(n_rules, n_facts, pattern="simple"):
    facts = {}
    rules = []
    for i in range(n_facts):
        facts[f"bool_{i}"] = (i % 2 == 0)
        facts[f"num_{i}"] = float(i * 10)

    for i in range(n_rules):
        name = f"bench_rule_{i}"
        action = f"ACT_{i % 20}"
        if pattern == "simple":
            cond = f"bool_{i % n_facts}"
        elif pattern == "and_cmp":
            cond = {
                "and": [
                    f"bool_{i % n_facts}",
                    {">": [f"num_{i % n_facts}", float((i % 100) * 10)]}
                ]
            }
        elif pattern == "nested":
            cond = {
                "and": [
                    f"bool_{i % n_facts}",
                    {
                        "or": [
                            {">": [f"num_{i % n_facts}", float((i % 100) * 10)]},
                            {"<": [f"num_{(i + 1) % n_facts}", float((i % 100) * 10 + 100)]}
                        ]
                    }
                ]
            }
        else:
            cond = f"bool_{i % n_facts}"

        rules.append({"name": name, "action": action, "if": cond})

    return {"facts": facts, "rules": rules}

def main():
    parser = argparse.ArgumentParser(description="Generate Vela rule JSON for benchmarks")
    parser.add_argument("--rules", type=int, default=100, help="Number of rules")
    parser.add_argument("--facts", type=int, default=50, help="Number of facts")
    parser.add_argument("--pattern", choices=["simple", "and_cmp", "nested"], default="and_cmp")
    parser.add_argument("--output", "-o", default=None)
    args = parser.parse_args()

    data = gen_rules(args.rules, args.facts, args.pattern)
    output = args.output or f"bench_rules_{args.rules}_{args.pattern}.json"
    with open(output, "w") as f:
        json.dump(data, f, indent=2)
    print(f"Generated {args.rules} rules, {args.facts} facts -> {output}")

if __name__ == "__main__":
    main()
