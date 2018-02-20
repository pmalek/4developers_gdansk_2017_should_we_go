package main

func f(ch <-chan string) {
	<-ch
}

func main() {
	ch := make(chan string)
	go f(ch)
	<-ch
}
