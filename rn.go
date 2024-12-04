package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"os"
	"os/exec"
	"strings"
)

func main() {
	editor := flag.String("e", "", "Set the editor")
	flag.Parse()
	fmt.Println(*editor)

	input := get_input()
	for i := range input {
		if input[i] == "" {
			input[i] = "."
		} else if _, err := os.Stat(input[i]); err != nil {
			log.Fatalf("Could not find %s\n", input[i])
		} else if strings.HasSuffix(input[i], "/") {
			input[i] = input[i][:len(input[i])-1]
		}

		temp_file := sh("mktemp")

		sh(fmt.Sprintf("ls -1 %s > %s", input[i], temp_file))
		initial := strings.Split(sh(fmt.Sprintf("cat %s", temp_file)), "\n")

		open_editor(*editor, temp_file)
		final := strings.Split(sh(fmt.Sprintf("cat %s", temp_file)), "\n")

		if len(initial) != len(final) {
			log.Fatal("Elements did not match.")
		}

		for j := range initial {
			old_name := fmt.Sprintf("%s/%s", input[i], initial[j])
			new_name := fmt.Sprintf("%s/%s", input[i], final[j])
			if old_name != new_name {
				if new_name == "" {
					log.Fatal("Cannot rename to blank.")
				}

				err := os.Rename(old_name, new_name)
				if err != nil {
					log.Fatalf("Error renaming file: %v", err)
				}

				fmt.Printf("%s -> %s\n", old_name, new_name)
			}
		}
	}
}

func get_input() []string {
	input := []string{}

	// Check input from arguments
	if flag.NArg() > 0 {
		for i := range flag.Args() {
			input = append(input, flag.Args()[i])
		}
		return input
	}

	// Check input from standard input or pipe
	stat, _ := os.Stdin.Stat()
	if stat.Mode()&os.ModeCharDevice == 0 {
		scanner := bufio.NewScanner(os.Stdin)
		for scanner.Scan() {
			input = append(input, scanner.Text())
		}
		return input
	}
	return []string{""}
}

func sh(command string) string {
	output, err := exec.Command("bash", "-c", fmt.Sprintf(command)).CombinedOutput()
	if err != nil {
		log.Fatalf("Error Running: %s\n", command)
		log.Fatal(err)
	}

	return string(output)
}

func open_editor(editor string, file string) {
	if editor == "" {
		editor = os.Getenv("EDITOR")
		if editor == "" {
			editor = "nvim"
		}
	}

	cmd := exec.Command("bash", "-c", fmt.Sprintf("%s %s", editor, file))
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	if err := cmd.Run(); err != nil {
		log.Fatalf("Error running editor: %v\n", err)
	}
}
