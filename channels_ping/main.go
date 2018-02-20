package main

import "fmt"
import "time"

type ball struct{ hits int }

func main() {
	table := make(chan *ball)
	go player("ping", table)
	go player("pong", table)

	table <- new(ball) // start the game
	time.Sleep(1 * time.Second)
	<-table // game over
}

func player(name string, table chan *ball) {
	for {
		ball := <-table
		ball.hits++
		fmt.Println(name, ball.hits)
		time.Sleep(100 * time.Millisecond)
		table <- ball
	}
}
