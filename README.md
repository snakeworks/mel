# mel

Small interpreted programming language implemented in C.

## Quick Start

```sh
git clone https://github.com/snakeworks/mel.git
cd mel
make
```

The executable will be available at `build/mel`.

## Usage

Run a source file with:

```sh
./build/mel /path/to/file
```

See the [`examples/`](examples/) directory for sample programs.

## Syntax

Mel uses a simple, C-like syntax: statements end in `;`, curly braces `{}` group blocks, and variables can be assigned with `=`. It supports expressions, conditionals, loops and functions.

```rs
message = "Hello, world!";
println(message);

if true {
    println("Mel is running");
} else {
    println("This never runs");
}

fn custom_print(str) {
    println("---");
    println(str);
    println("---");
}

for i in 5 {
    custom_print(i);
}
```

## TODO
- Add more examples
- Expand upon README
