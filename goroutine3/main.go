package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

var wg sync.WaitGroup

func f(i int) {
	time.Sleep(time.Duration(rand.Intn(10000)) * time.Millisecond)
	fmt.Printf("%d finished sleeping\n", i)
	wg.Done()
}

func main() {
	wg.Add(10)
	for i := 1; i <= 10; i++ {
		go f(i)
	}
	wg.Wait()
}
