use clap::Parser;
use sp1_sdk::blocking::{ProveRequest, Prover, ProverClient, SP1Stdin};
use sp1_sdk::ProvingKey;
use sp1_sdk::include_elf;

const MONAD_ELF: sp1_sdk::Elf = include_elf!("monad-sp1-program");

#[derive(Parser)]
#[command(about = "Monad EVM Client - SP1 host/prover")]
struct Args {
    /// Path to input binary file
    #[arg(short, long)]
    input: String,

    /// Generate a proof instead of just executing
    #[arg(long)]
    prove: bool,
}

fn main() {
    let args = Args::parse();

    let input = std::fs::read(&args.input).unwrap_or_else(|e| {
        eprintln!("Error reading {}: {}", args.input, e);
        std::process::exit(1);
    });

    println!("Monad EVM Client (SP1) - Blockchain Test Execution");
    println!("Test data size: {} bytes", input.len());

    let mut stdin = SP1Stdin::new();
    stdin.write_vec(input);

    let client = ProverClient::from_env();

    if args.prove {
        let pk = client.setup(MONAD_ELF).expect("setup failed");
        let proof = client
            .prove(&pk, stdin)
            .run()
            .expect("proving failed");
        client
            .verify(&proof, pk.verifying_key(), None)
            .expect("verification failed");
        println!("Proof generated and verified successfully");
    } else {
        let (mut output, report) = client
            .execute(MONAD_ELF, stdin)
            .run()
            .expect("execution failed");
        let result = output.read::<u32>();

        if result == 1 {
            println!("Test passed");
        } else {
            println!("Test failed");
        }
        println!("Cycles: {}", report.total_instruction_count());
    }
}
