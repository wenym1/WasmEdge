use rand::Rng;
use std::fmt;

struct Matrix {
    row: usize,
    col: usize,
    data: Vec<f32>,
}


impl fmt::Display for Matrix {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> fmt::Result {
        if self.row < 5 && self.col < 5 {
            for i in 0..self.row {
                for j in 0..self.col {
                    write!(f, "{:5} ", self.get(i, j))?;
                }
                writeln!(f, "")?;
            }
            write!(f, "")
        } else {
            write!(f, "Matrix({}, {})", self.row, self.col)
        }
    }
}

impl Matrix {

    fn get(&self, i: usize, j: usize) -> f32 {
        if i < self.row && j < self.col {
            self.data[i * self.col + j]
        } else {
            panic!("matrix index not within bound: matrix({}, {}), i: {}, j: {}", self.row, self.col, i, j);
        }
    }

    fn new_random(row: usize, col: usize, rng: &mut rand::rngs::ThreadRng) -> Matrix {
        let mut data = Vec::new();
        for _ in 0..row {
            for _ in 0..col {
                data.push(rng.gen::<f32>())
            }
        }
        Matrix {
            row: row,
            col: col,
            data: data
        }
    }

    fn multiple(first: &Matrix, second: &Matrix) -> Result<Matrix, String> {
        if first.col != second.row {
            Err(format!("the col number of the first matrix does not the row number of the second matrix: {} and {}", first.col, second.row).to_owned())
        } else {
            let mut data = Vec::new();
            let row = first.row;
            let internal_dim = first.col;
            let col = second.col;
            for i in 0..row {
                for j in 0..col {
                    let mut sum = 0.0;
                    for k in 0..internal_dim {
                        sum += first.get(i, k) * second.get(k, j)
                    }
                    data.push(sum);
                }
            }
            Ok(Matrix {
                row: first.row,
                col: second.col,
                data: data,
            })
        }
    }

    #[allow(dead_code)]
    fn empty() -> Matrix {
        Matrix {
            row: 0,
            col: 0,
            data: vec![],
        }
    }
}

fn main() {
    let row_count = std::env::args()
        .nth(1)
        .expect("no final row number given")
        .parse::<usize>()
        .unwrap();
    let internal_count = std::env::args()
        .nth(2)
        .expect("no internal dimension number given")
        .parse::<usize>()
        .unwrap();
    let col_count = std::env::args()
        .nth(3)
        .expect("no final col number given")
        .parse::<usize>()
        .unwrap();

    let mut rng = rand::thread_rng();

    let first = Matrix::new_random(row_count, internal_count, &mut rng);
    let second = Matrix::new_random(internal_count, col_count, &mut rng);

    match Matrix::multiple(&first, &second) {
        Ok(result) => {
            println!("first matrix: \n{}", first);
            println!("second matrix: \n{}", second);
            println!("result matrix: \n{}", result);
        }
        Err(err_msg) => {
            println!("get error in matrix mul: {}", err_msg);
        }
    }
}
