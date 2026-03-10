//! Assignment 4 – Linked List Memory Experiment (Rust)
//!
//! Build : rustc -o FilipRust FilipRust.rs
//! Run   : ./FilipRust
//!
//! Reads Council_Member_Expenses CSV into a singly-linked list.
//! Each Node holds a Row (council-member name + expense amount).
//! After loading, prints VmSize from /proc/self/status.
//! Also runs self-tests for insert_front and list_to_string.

use std::fs::File;
use std::io::{self, BufRead, BufReader};

const FILENAME: &str = "../2006Assignment3/Council_Member_Expenses_20260224.csv";

// ── Data types ────────────────────────────────────────────────────

/// One expense record: at least 2 data fields (name + amount).
struct Row {
    name:   String,
    amount: f64,
}

/// Singly-linked list node.
struct Node {
    data: Row,
    next: Option<Box<Node>>,
}

// ── Linked-list functions ─────────────────────────────────────────

/// Insert a new Row at the front of the list; returns new head.
fn insert_front(head: Option<Box<Node>>, name: &str, amount: f64) -> Option<Box<Node>> {
    Some(Box::new(Node {
        data: Row { name: name.to_string(), amount },
        next: head,
    }))
}

/// Return "[name/$amt, ...]" representation of the list.
fn list_to_string(head: &Option<Box<Node>>) -> String {
    let mut parts: Vec<String> = Vec::new();
    let mut cur = head;
    while let Some(node) = cur {
        parts.push(format!("{}/${:.2}", node.data.name, node.data.amount));
        cur = &node.next;
    }
    format!("[{}]", parts.join(", "))
}

// ── Memory reporting ──────────────────────────────────────────────

fn print_memory_usage() {
    match std::fs::read_to_string("/proc/self/status") {
        Ok(contents) => {
            for line in contents.lines() {
                if line.starts_with("VmSize:") {
                    println!("Memory usage (VmSize): {}", line[7..].trim());
                    return;
                }
            }
            eprintln!("VmSize not found in /proc/self/status");
        }
        Err(_) => eprintln!("Cannot read /proc/self/status (not Linux?)"),
    }
}

// ── Self-tests ────────────────────────────────────────────────────

fn run_tests() {
    println!("=== Linked List Tests ===");

    // Test 1: empty list
    let head: Option<Box<Node>> = None;
    println!("Empty list        : {}", list_to_string(&head)); // []

    // Test 2: single insert
    let head = insert_front(head, "Alice", 100.00);
    println!("After insert Alice: {}", list_to_string(&head)); // [Alice/$100.00]

    // Test 3: multiple inserts – each goes to the front
    let head = insert_front(head, "Bob",   50.25);
    let head = insert_front(head, "Carol", 200.00);
    println!("After Bob, Carol  : {}", list_to_string(&head)); // [Carol/$200.00, Bob/$50.25, Alice/$100.00]

    // Test 4: verify order
    if let Some(ref n) = head {
        println!("Head name         : {}  (expected Carol)", n.data.name);
        if let Some(ref n2) = n.next {
            println!("Head->next name   : {}  (expected Bob)", n2.data.name);
        }
    }

    // Test 5: verify tail is still reachable
    let mut cur = &head;
    while let Some(ref node) = cur {
        if node.next.is_none() {
            println!("Tail name         : {}  (expected Alice)", node.data.name);
            break;
        }
        cur = &node.next;
    }

    println!("=========================\n");
}

// ── CSV helpers (reused from Assignment 3) ────────────────────────

fn parse_amount(s: &str) -> Option<f64> {
    let cleaned: String = s.chars().filter(|&c| c != '$' && c != ',' && c != ' ').collect();
    if cleaned.is_empty() { return None; }
    cleaned.parse::<f64>().ok()
}

fn parse_csv_fields(record: &str) -> Vec<String> {
    let mut fields = Vec::new();
    let mut chars = record.chars().peekable();
    loop {
        let mut field = String::new();
        if chars.peek() == Some(&'"') {
            chars.next(); // consume opening quote
            loop {
                match chars.next() {
                    None => break,
                    Some('"') => {
                        if chars.peek() == Some(&'"') {
                            chars.next(); // escaped double-quote
                            field.push('"');
                        } else {
                            break; // closing quote
                        }
                    }
                    Some(c) => field.push(c),
                }
            }
        } else {
            loop {
                match chars.peek() {
                    None | Some('\n') | Some('\r') | Some(',') => break,
                    Some(_) => field.push(chars.next().unwrap()),
                }
            }
        }
        fields.push(field);
        match chars.peek() {
            Some(',') => { chars.next(); } // consume separator
            _ => break,
        }
    }
    fields
}

/// Read one complete logical CSV record (handles quoted newlines).
fn read_csv_record<R: BufRead>(reader: &mut R) -> io::Result<Option<String>> {
    let mut record = String::new();
    let mut in_quote = false;
    loop {
        let mut line = String::new();
        let bytes = reader.read_line(&mut line)?;
        if bytes == 0 {
            return if record.is_empty() { Ok(None) } else { Ok(Some(record)) };
        }
        let mut chars = line.chars().peekable();
        while let Some(c) = chars.next() {
            match c {
                '\r' => {}
                '\n' if !in_quote => {
                    if record.is_empty() { break; } // blank line – skip
                    return Ok(Some(record));
                }
                '"' if in_quote => {
                    if chars.peek() == Some(&'"') {
                        chars.next();
                        record.push('"');
                        record.push('"');
                    } else {
                        in_quote = false;
                        record.push('"');
                    }
                }
                '"' => { in_quote = true; record.push('"'); }
                other => record.push(other),
            }
        }
    }
}

// ── Main ──────────────────────────────────────────────────────────

fn main() {
    run_tests();

    let file = match File::open(FILENAME) {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Error: cannot open '{}': {}", FILENAME, e);
            return;
        }
    };

    let mut reader = BufReader::new(file);

    // Skip header
    match read_csv_record(&mut reader) {
        Ok(None) => { eprintln!("Error: file is empty."); return; }
        Err(e)   => { eprintln!("Error reading header: {}", e); return; }
        Ok(Some(_)) => {}
    }

    let mut head: Option<Box<Node>> = None;

    loop {
        let record = match read_csv_record(&mut reader) {
            Ok(Some(r)) => r,
            Ok(None)    => break,
            Err(_)      => continue,
        };

        let fields = parse_csv_fields(&record);
        if fields.len() != 10 { continue; }

        let name      = fields[2].trim().to_string();
        let amount_raw = fields[8].trim().to_string();
        if name.is_empty() || amount_raw.is_empty() { continue; }

        if let Some(amount) = parse_amount(&amount_raw) {
            head = insert_front(head, &name, amount);
        }
    }

    println!("CSV loaded into linked list.");
    print_memory_usage();

    // Print first 5 nodes as a sample
    println!("\nFirst 5 nodes:");
    let mut cur = &head;
    let mut count = 0;
    while let Some(node) = cur {
        if count >= 5 { break; }
        println!("  {}  ${:.2}", node.data.name, node.data.amount);
        cur = &node.next;
        count += 1;
    }
}
