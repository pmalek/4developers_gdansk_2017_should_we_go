package main

import "fmt"

type runner interface {
	Run() int
}

func howFar(r runner) {
	fmt.Printf("This runner ran %d km\n", r.Run())
}

type human struct{}

func (h human) Run() int {
	return 10
}

func main() {
	h := human{}
	howFar(h)
}
