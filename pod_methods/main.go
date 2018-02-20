package main

import "fmt"

type temperature float64

func (t temperature) String() string {
	return fmt.Sprintf("%.2f ℃", t)
}

func main() {
	t := temperature(25.0)
	fmt.Println(t)
}
