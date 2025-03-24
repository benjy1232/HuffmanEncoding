use std::collections::HashMap;
use std::env;
use std::fs;
use std::io::{BufReader, Read};

struct BitString {
    bit_string: u64,
    len: i32,
    character: u8,
}

fn read_64_bytes(file_buf: &mut BufReader<fs::File>) -> Result<u64, ()> {
    let mut u64_buffer: [u8; 8] = [0; 8];
    let bytes_read = file_buf
        .read(&mut u64_buffer)
        .expect("Unable to read u64 from buffer");
    if bytes_read < 8 {
        eprintln!("Not enought bytes read for decoded file len");
        return Err(());
    }
    Ok(u64::from_le_bytes(u64_buffer))
}

fn get_dict_entries(encoded_buffer: &mut BufReader<fs::File>) -> HashMap<u64, Vec<BitString>> {
    let dict_len = read_64_bytes(encoded_buffer).expect("Unable to read dict len");
    let bit_string_size = size_of::<BitString>() as u64;
    let mut bit_string_map: HashMap<u64, Vec<BitString>> = HashMap::new();
    let mut dict_entries = dict_len / bit_string_size;
    let mut bit_string_buf = [0u8; size_of::<BitString>()];
    while dict_entries > 0 {
        let bytes_read = encoded_buffer
            .read(&mut bit_string_buf)
            .expect("Unable to read all bytes for this BitString Entry");
        if bytes_read < 16 {
            eprintln!("Unable to read bit_string");
            return bit_string_map;
        }
        let bit_string = BitString {
            bit_string: u64::from_le_bytes(bit_string_buf[..8].try_into().unwrap()),
            len: i32::from_le_bytes(bit_string_buf[8..12].try_into().unwrap()),
            character: bit_string_buf[12],
        };

        if bit_string_map.contains_key(&bit_string.bit_string) {
            bit_string_map
                .get_mut(&bit_string.bit_string)
                .unwrap()
                .push(bit_string);
        } else {
            bit_string_map.insert(bit_string.bit_string, vec![bit_string]);
        }
        dict_entries -= 1;
    }
    bit_string_map
}

fn decode_byte(
    byte: u8,
    cbs: &mut BitString,
    bit_string_map: &HashMap<u64, Vec<BitString>>,
    decoded_bytes: &mut Vec<u8>,
    decoded_file_len: u64
) {
    let mut i = 7;
    while i >= 0 {
        cbs.bit_string = cbs.bit_string << 1;
        let bit_to_set = (byte >> i) & 1;
        cbs.bit_string |= bit_to_set as u64;
        cbs.len += 1;
        if bit_string_map.contains_key(&cbs.bit_string) {
            for bs in bit_string_map.get(&cbs.bit_string).unwrap() {
                if bs.len == cbs.len {
                    // append to output
                    cbs.len = 0;
                    cbs.bit_string = 0;
                    decoded_bytes.push(bs.character);
                    if decoded_bytes.len() == decoded_file_len as usize {
                        return;
                    }
                }
            }
        }
        i -= 1;
    }
}

fn decode_file(encoded_buffer: &mut BufReader<fs::File>) -> i32 {
    let decoded_file_len = read_64_bytes(encoded_buffer).expect("Unable to read decoded_file_len");
    let bit_string_map = get_dict_entries(encoded_buffer);
    if bit_string_map.len() == 0 {
        return 1;
    }
    let mut cbs = BitString {
        bit_string: 0,
        len: 0,
        character: 0,
    };

    let mut decoded_byte_list: Vec<u8> = vec![];
    for b in encoded_buffer.bytes() {
        let b = b.expect("Unable to read byte");
        decode_byte(b, &mut cbs, &bit_string_map, &mut decoded_byte_list, decoded_file_len);
    }

    let output = String::from_utf8(decoded_byte_list).unwrap();
    print!("{output}");

    0
}

fn main() {
    if env::args().count() < 2 {
        eprint!("Need file to decode");
        std::process::exit(1)
    }
    let file_path = env::args().nth(1).unwrap();
    let encoded_file = fs::File::open(file_path).expect("File exit");
    let mut encoded_file = BufReader::new(encoded_file);
    std::process::exit(decode_file(&mut encoded_file))
}
