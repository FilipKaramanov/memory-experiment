"""
Assignment 4 – Linked List Memory Experiment (Python)
Run: python3 FilipPython.py

Reads Council_Member_Expenses CSV into a singly-linked list.
Each Node holds a Row object (council-member name + expense amount).
After loading, prints VmSize from /proc/self/status.
Also runs self-tests for insert_front and list_to_string.
"""

import csv
import sys

FILENAME = "../2006Assignment3/Council_Member_Expenses_20260224.csv"

# ── Data classes ──────────────────────────────────────────────────


class Row:
    """One expense record: at least 2 data fields (name + amount)."""

    def __init__(self, name: str, amount: float):
        self.name = name
        self.amount = amount


class Node:
    """Singly-linked list node that holds a Row."""

    def __init__(self, row: "Row", next_node: "Node | None" = None):
        self.data = row
        self.next = next_node


# ── Linked-list functions ──────────────────────────────────────────


def insert_front(head: "Node | None", name: str, amount: float) -> Node:
    """Insert a new Row at the front of the list; return new head."""
    return Node(Row(name, amount), head)


def list_to_string(head: "Node | None") -> str:
    """Return '[name/$amt, ...]' representation of the list."""
    items = []
    cur = head
    while cur is not None:
        items.append(f"{cur.data.name}/${cur.data.amount:.2f}")
        cur = cur.next
    return "[" + ", ".join(items) + "]"


# ── Memory reporting ───────────────────────────────────────────────


def print_memory_usage() -> None:
    try:
        with open("/proc/self/status") as fh:
            for line in fh:
                if line.startswith("VmSize:"):
                    parts = line.split()
                    print(f"Memory usage (VmSize): {parts[1]} {parts[2]}")
                    return
    except OSError:
        print("Cannot read /proc/self/status (not Linux?)")


# ── Self-tests ─────────────────────────────────────────────────────


def run_tests() -> None:
    print("=== Linked List Tests ===")

    # Test 1: empty list
    head = None
    print(f"Empty list        : {list_to_string(head)}")   # []

    # Test 2: single insert
    head = insert_front(head, "Alice", 100.00)
    print(f"After insert Alice: {list_to_string(head)}")   # [Alice/$100.00]

    # Test 3: multiple inserts – each goes to the front
    head = insert_front(head, "Bob",   50.25)
    head = insert_front(head, "Carol", 200.00)
    print(f"After Bob, Carol  : {list_to_string(head)}")   # [Carol/$200.00, Bob/$50.25, Alice/$100.00]

    # Test 4: verify order
    print(f"Head name         : {head.data.name}  (expected Carol)")
    print(f"Head->next name   : {head.next.data.name}  (expected Bob)")

    # Test 5: verify tail is still reachable
    cur = head
    while cur.next:
        cur = cur.next
    print(f"Tail name         : {cur.data.name}  (expected Alice)")

    print("=========================\n")


# ── CSV helper (reused from Assignment 3) ─────────────────────────


def parse_amount(raw: str) -> float:
    cleaned = raw.strip().replace("$", "").replace(",", "")
    if not cleaned:
        raise ValueError("empty amount string")
    return float(cleaned)


# ── Main ───────────────────────────────────────────────────────────


def main() -> None:
    run_tests()

    head = None
    try:
        with open(FILENAME, encoding="utf-8", newline="") as fh:
            reader = csv.reader(fh)
            try:
                next(reader)  # skip header
            except StopIteration:
                print("Error: file is empty.")
                sys.exit(1)

            for row in reader:
                if len(row) != 10:
                    continue
                name = row[2].strip()
                amount_raw = row[8].strip()
                if not name or not amount_raw:
                    continue
                try:
                    amount = parse_amount(amount_raw)
                except ValueError:
                    continue
                head = insert_front(head, name, amount)

    except FileNotFoundError:
        print(f"Error: cannot open '{FILENAME}'.")
        sys.exit(1)

    print("CSV loaded into linked list.")
    print_memory_usage()

    # Print first 5 nodes as a sample
    print("\nFirst 5 nodes:")
    cur = head
    count = 0
    while cur and count < 5:
        print(f"  {cur.data.name}  ${cur.data.amount:.2f}")
        cur = cur.next
        count += 1


if __name__ == "__main__":
    main()
