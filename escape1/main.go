package main

type youShallNotEscape struct {
	num int
}

func main() {
	y := new(youShallNotEscape)
	y.num = 2
}
