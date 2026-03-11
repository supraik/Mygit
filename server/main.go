// ============================================================================
// MYGIT SERVER - A Go Learning Journey
// ============================================================================
// This file teaches you Go through building a real Git server!
// Read the comments carefully - they explain Go concepts as we use them.
// ============================================================================

// LESSON 1: PACKAGES
// ------------------
// Every Go file starts with a package declaration.
// "main" is special - it tells Go this is an executable program (not a library).
// Think of it like: int main() in C++
package main

// LESSON 2: IMPORTS
// -----------------
// Go uses "import" to bring in external packages (like #include in C++).
// Standard library packages don't need installation - they come with Go.
// Multiple imports are grouped in parentheses.
import (
	"encoding/json" // For JSON encoding/decoding (API responses)
	"fmt"           // Format package: printing, string formatting (like printf)
	"io"            // Input/Output interfaces
	"log"           // Logging with timestamps
	"net/http"      // HTTP server - Go's built-in web server (no external deps!)
	"os"            // Operating system functions: file operations, env vars
	"path/filepath" // Cross-platform file path manipulation
	"sort"          // Sorting slices
	"strings"       // String manipulation functions
)

// LESSON 3: CONSTANTS
// -------------------
// "const" declares constants (immutable values).
// Go infers the type from the value, or you can specify it explicitly.
const (
	repoPath    = ".mygit"            // Where our Git data lives
	objectsDir  = ".mygit/objects"    // Git objects (commits, trees, blobs)
	refsDir     = ".mygit/refs/heads" // Branch references
	defaultPort = ":8080"             // Colon prefix = listen on all interfaces
)

// ============================================================================
// LESSON 4: THE MAIN FUNCTION
// ============================================================================
// "func" keyword declares a function.
// "main" is the entry point - execution starts here.
// Go doesn't use semicolons at line ends (the compiler adds them).
func main() {
	// LESSON 5: SHORT VARIABLE DECLARATION
	// ------------------------------------
	// := is "short variable declaration" - declares AND initializes
	// Go infers the type automatically. This is idiomatic Go.
	// Equivalent to: var port string = getPort()
	port := getPort() //custom function defined at line 100

	// LESSON 6: PRINTING OUTPUT
	// -------------------------
	// fmt.Printf works like C's printf. %s = string placeholder.
	// \n = newline (same as C/C++)
	fmt.Printf(" MyGit Server starting on port %s\n", port)
	fmt.Println("=" + strings.Repeat("=", 50))

	// LESSON 7: REGISTERING HTTP HANDLERS
	// ------------------------------------
	// handle health is a standard library function that
	// http.HandleFunc maps URL paths to handler functions.
	// When someone visits /refs, Go calls handleListRefs.
	// This is similar to routing in web frameworks.
	http.HandleFunc("/refs", handleListRefs)
	http.HandleFunc("/ref/", handleRef)       // Note: trailing / catches /ref/*
	http.HandleFunc("/object/", handleObject) // Catches /object/*
	http.HandleFunc("/health", handleHealth)  // Health check for deployment

	// Frontend & JSON API routes
	http.HandleFunc("/", handleFrontend)                // Serves the web dashboard
	http.HandleFunc("/api/branches", handleAPIBranches) // JSON list of branches
	http.HandleFunc("/api/log/", handleAPILog)          // Commit history for a branch
	http.HandleFunc("/api/tree/", handleAPITree)        // File tree for a commit
	http.HandleFunc("/api/blob/", handleAPIBlob)        // File content by hash
	http.HandleFunc("/api/commit/", handleAPICommit)    // Commit details

	// Print helpful info for the user
	fmt.Println("📡 Available endpoints:")
	fmt.Println("   GET  /refs        - List all branches")
	fmt.Println("   GET  /ref/{name}  - Get branch commit hash")
	fmt.Println("   GET  /object/{hash} - Download git object")
	fmt.Println("   POST /object/{hash} - Upload git object")
	fmt.Println("   POST /ref/{name}  - Update branch reference")
	fmt.Println("   GET  /health      - Health check")
	fmt.Println(strings.Repeat("=", 51))

	// LESSON 8: ERROR HANDLING (IMPORTANT!)
	// -------------------------------------
	// Go doesn't have exceptions. Functions return errors explicitly.
	// http.ListenAndServe returns an error if it fails.
	// log.Fatal prints the error and exits the program (os.Exit(1))
	//
	// This pattern is EVERYWHERE in Go:
	//   result, err := someFunction()
	//   if err != nil { handle error }
	log.Fatal(http.ListenAndServe(port, nil))
}

// ============================================================================
// LESSON 9: FUNCTION DEFINITIONS
// ============================================================================
// Syntax: func name(params) returnType { body }
// Return type comes AFTER parameters (opposite of C/C++)
func getPort() string {
	// LESSON 10: ENVIRONMENT VARIABLES
	// ---------------------------------
	// os.Getenv reads environment variables (useful for deployment!)
	// If PORT is set, use it; otherwise use default.
	// This is how cloud platforms (Heroku, Railway, etc.) configure ports.
	if port := os.Getenv("PORT"); port != "" {
		return ":" + port
	}
	return defaultPort
}

// ============================================================================
// LESSON 11: HTTP HANDLERS
// ============================================================================
// HTTP handlers have a specific signature:
//
//	func(w http.ResponseWriter, r *http.Request)
//
// - w: where you write your response (the output)
// - r: the incoming request (contains URL, method, headers, body)
//
// LESSON 12: POINTERS
// -------------------
// *http.Request means "pointer to Request" (like C++)
// Go has pointers but NO pointer arithmetic (safer than C++)
// & gets address, * dereferences (same as C++)
func handleHealth(w http.ResponseWriter, r *http.Request) {
	// LESSON 13: WRITING HTTP RESPONSES
	// ----------------------------------
	// w.Write() sends bytes to the client
	// []byte("text") converts string to byte slice
	//
	// LESSON 14: SLICES
	// -----------------
	// []byte is a "slice" - a dynamic view into an array.
	// Slices are like C++ vectors but built into the language.
	// []Type creates a slice of Type.
	w.Write([]byte("OK"))
}

// ============================================================================
// HANDLER: List all branches (refs)
// ============================================================================
func handleListRefs(w http.ResponseWriter, r *http.Request) {
	// LESSON 15: METHOD CHECKING
	// --------------------------
	// r.Method contains "GET", "POST", etc.
	// We only allow GET for listing refs.
	if r.Method != http.MethodGet {
		// LESSON 16: HTTP STATUS CODES
		// ----------------------------
		// http.Error sends an error response with status code.
		// StatusMethodNotAllowed = 405
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return // Exit the function early
	}

	// LESSON 17: CALLING OTHER FUNCTIONS & ERROR HANDLING
	// ----------------------------------------------------
	// Functions can return multiple values!
	// This is a KEY Go feature - commonly (result, error)
	refs, err := listAllRefs()
	if err != nil {
		// != means "not equal" (same as C++)
		// nil is Go's "null" - means "no value"
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// LESSON 18: RESPONSE HEADERS
	// ---------------------------
	// w.Header().Set() sets HTTP headers
	// Content-Type tells the client what format the data is in
	w.Header().Set("Content-Type", "text/plain")

	// LESSON 19: ITERATING WITH range
	// --------------------------------
	// "range" iterates over collections. It returns (index/key, value).
	// Here: ref is the key (branch name), hash is the value (commit hash)
	// This is like: for (auto& [ref, hash] : refs) in C++17
	for ref, hash := range refs {
		// fmt.Fprintf writes formatted output to any io.Writer
		// %s = string substitution (like printf)
		fmt.Fprintf(w, "%s %s\n", hash, ref)
	}
}

// ============================================================================
// HANDLER: Get or update a specific ref (branch)
// ============================================================================
func handleRef(w http.ResponseWriter, r *http.Request) {
	refName := strings.TrimPrefix(r.URL.Path, "/ref/")

	if refName == "" {
		http.Error(w, "Reference name required", http.StatusBadRequest)
		return
	}

	// Reject path traversal attempts
	if strings.Contains(refName, "..") || strings.Contains(refName, "/") || strings.Contains(refName, "\\") {
		http.Error(w, "Invalid reference name", http.StatusBadRequest)
		return
	}

	// LESSON 21: SWITCH STATEMENTS
	// ----------------------------
	// Go's switch is cleaner than C/C++:
	// - No "break" needed (doesn't fall through by default)
	// - Can switch on strings (not just integers)
	// - Use "fallthrough" keyword if you DO want to fall through
	switch r.Method {
	case http.MethodGet:
		hash, err := getRef(refName)
		if err != nil {
			http.Error(w, "Reference not found", http.StatusNotFound)
			return
		}
		w.Write([]byte(hash))

	case http.MethodPost:
		// LESSON 22: READING REQUEST BODY
		// --------------------------------
		// r.Body is an io.ReadCloser - a stream you can read from
		// io.ReadAll reads everything into memory (careful with large files!)
		body, err := io.ReadAll(r.Body)
		if err != nil {
			http.Error(w, "Failed to read body", http.StatusBadRequest)
			return
		}

		// LESSON 23: TYPE CONVERSION
		// --------------------------
		// string(body) converts []byte to string
		// strings.TrimSpace removes leading/trailing whitespace
		hash := strings.TrimSpace(string(body))

		if err := updateRef(refName, hash); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		// Log the action (helpful for debugging)
		log.Printf("Updated ref: %s -> %s", refName, hash)
		w.Write([]byte("OK"))

	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

// ============================================================================
// HANDLER: Get or store git objects
// ============================================================================
func handleObject(w http.ResponseWriter, r *http.Request) {
	hash := strings.TrimPrefix(r.URL.Path, "/object/")

	if hash == "" || len(hash) < 3 {
		http.Error(w, "Invalid object hash", http.StatusBadRequest)
		return
	}

	// Reject path traversal attempts and validate hex
	if strings.Contains(hash, "..") || strings.Contains(hash, "/") || strings.Contains(hash, "\\") {
		http.Error(w, "Invalid object hash", http.StatusBadRequest)
		return
	}

	switch r.Method {
	case http.MethodGet:
		content, err := readObject(hash)
		if err != nil {
			http.Error(w, "Object not found", http.StatusNotFound)
			return
		}
		// LESSON 24: SETTING CONTENT TYPE FOR BINARY DATA
		// ------------------------------------------------
		// application/octet-stream = raw binary data
		w.Header().Set("Content-Type", "application/octet-stream")
		w.Write(content)

	case http.MethodPost:
		body, err := io.ReadAll(r.Body)
		if err != nil {
			http.Error(w, "Failed to read body", http.StatusBadRequest)
			return
		}

		if err := writeObject(hash, body); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		log.Printf("Stored object: %s", hash)
		w.Write([]byte("OK"))

	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

// ============================================================================
// LESSON 25: MAPS (LIKE DICTIONARIES/HASHMAPS)
// ============================================================================
// map[KeyType]ValueType declares a map
// This function returns a map AND an error
func listAllRefs() (map[string]string, error) {
	// LESSON 26: MAKE FUNCTION
	// ------------------------
	// make() creates and initializes built-in types:
	// - make(map[K]V) creates an empty map
	// - make([]T, len, cap) creates a slice
	// - make(chan T) creates a channel (for concurrency)
	refs := make(map[string]string)

	// LESSON 27: FILE SYSTEM OPERATIONS
	// ----------------------------------
	// os.Stat gets file/directory info
	// os.IsNotExist checks if the error means "file doesn't exist"
	if _, err := os.Stat(refsDir); os.IsNotExist(err) {
		// Directory doesn't exist = no refs yet = return empty map
		return refs, nil
	}

	// LESSON 28: READING DIRECTORIES
	// -------------------------------
	// os.ReadDir returns a slice of directory entries
	entries, err := os.ReadDir(refsDir)
	if err != nil {
		return nil, err // Return nil map and the error
	}

	// Iterate over all files in the refs directory
	for _, entry := range entries {
		// LESSON 29: SKIPPING WITH CONTINUE
		// ----------------------------------
		// "continue" skips to the next iteration (same as C++)
		if entry.IsDir() {
			continue // Skip subdirectories
		}

		// LESSON 30: FILEPATH OPERATIONS
		// -------------------------------
		// filepath.Join safely joins path segments with the right separator
		// This is cross-platform (handles / vs \ automatically)
		refPath := filepath.Join(refsDir, entry.Name())

		// LESSON 31: READING FILES
		// ------------------------
		// os.ReadFile reads entire file into memory (simple but memory-heavy)
		// For large files, use os.Open + bufio.Scanner
		content, err := os.ReadFile(refPath)
		if err != nil {
			continue // Skip files we can't read
		}

		// Build the full ref name (e.g., "refs/heads/main")
		refName := "refs/heads/" + entry.Name()
		refs[refName] = strings.TrimSpace(string(content))
	}

	return refs, nil // Return the map and no error
}

// Get a specific ref's commit hash
func getRef(name string) (string, error) {
	refPath := filepath.Join(refsDir, name)

	content, err := os.ReadFile(refPath)
	if err != nil {
		return "", err
	}

	return strings.TrimSpace(string(content)), nil
}

// Update a ref to point to a new commit
func updateRef(name string, hash string) error {
	// LESSON 32: CREATING DIRECTORIES
	// --------------------------------
	// os.MkdirAll creates a directory and all parent directories
	// 0755 is the permission mode (rwxr-xr-x in Unix)
	// This function is safe to call even if directory exists
	if err := os.MkdirAll(refsDir, 0755); err != nil {
		return err
	}

	refPath := filepath.Join(refsDir, name)

	// LESSON 33: WRITING FILES
	// ------------------------
	// os.WriteFile writes data to a file (creates or overwrites)
	// 0644 = rw-r--r-- permissions
	return os.WriteFile(refPath, []byte(hash), 0644)
}

// Read a git object by its hash
func readObject(hash string) ([]byte, error) {
	// Git stores objects in directories named by first 2 chars of hash
	// e.g., hash "abc123..." is stored in objects/ab/c123...
	dir := hash[:2]  // First 2 characters (slice syntax!)
	file := hash[2:] // Everything after first 2 characters

	objPath := filepath.Join(objectsDir, dir, file)
	return os.ReadFile(objPath)
}

// Write a git object
func writeObject(hash string, content []byte) error {
	dir := hash[:2]
	file := hash[2:]

	dirPath := filepath.Join(objectsDir, dir)

	// Create the directory if it doesn't exist
	if err := os.MkdirAll(dirPath, 0755); err != nil {
		return err
	}

	objPath := filepath.Join(dirPath, file)
	return os.WriteFile(objPath, content, 0644)
}

// ============================================================================
// LESSON 34: BONUS - JSON API ENDPOINT (Optional)
// ============================================================================
// This shows how to return JSON responses - useful for modern APIs

// LESSON 35: STRUCT TAGS
// ----------------------
// Structs can have "tags" - metadata in backticks after the type.
// `json:"name"` tells the JSON encoder to use "name" as the key.
// This is how Go handles JSON serialization - no external annotations!
type RefInfo struct {
	Name string `json:"name"`
	Hash string `json:"hash"`
}

type RefsResponse struct {
	Refs []RefInfo `json:"refs"`
}

// Example JSON endpoint (not currently registered, but here for learning)
func handleListRefsJSON(w http.ResponseWriter, r *http.Request) {
	refs, err := listAllRefs()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// Build response struct
	response := RefsResponse{
		Refs: make([]RefInfo, 0, len(refs)), // Slice with len=0, capacity=len(refs)
	}

	for name, hash := range refs {
		// LESSON 36: APPEND
		// -----------------
		// append() adds elements to a slice and returns the new slice
		// Slices grow automatically (like C++ vector)
		response.Refs = append(response.Refs, RefInfo{Name: name, Hash: hash})
	}

	// LESSON 37: JSON ENCODING
	// ------------------------
	// Set content type for JSON
	w.Header().Set("Content-Type", "application/json")

	// json.NewEncoder creates an encoder that writes to w
	// Encode() converts the struct to JSON and writes it
	json.NewEncoder(w).Encode(response)
}

// ============================================================================
// FRONTEND & API HANDLERS
// ============================================================================

// Serve the web dashboard
func handleFrontend(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/" {
		http.NotFound(w, r)
		return
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Write([]byte(frontendHTML))
}

// JSON API: list branches
func handleAPIBranches(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	refs, err := listAllRefs()
	if err != nil {
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}

	type BranchInfo struct {
		Name string `json:"name"`
		Hash string `json:"hash"`
	}

	branches := make([]BranchInfo, 0)
	for name, hash := range refs {
		shortName := strings.TrimPrefix(name, "refs/heads/")
		branches = append(branches, BranchInfo{Name: shortName, Hash: hash})
	}

	sort.Slice(branches, func(i, j int) bool {
		return branches[i].Name < branches[j].Name
	})

	json.NewEncoder(w).Encode(branches)
}

// Parse a commit object's body into structured data
type CommitInfo struct {
	Hash      string   `json:"hash"`
	Tree      string   `json:"tree"`
	Parents   []string `json:"parents"`
	Author    string   `json:"author"`
	Committer string   `json:"committer"`
	Message   string   `json:"message"`
}

func parseCommitObject(hash string, data []byte) (*CommitInfo, error) {
	content := string(data)
	nullIdx := strings.Index(content, "\x00")
	if nullIdx == -1 {
		return nil, fmt.Errorf("invalid object format")
	}
	body := content[nullIdx+1:]

	info := &CommitInfo{Hash: hash}
	parts := strings.SplitN(body, "\n\n", 2)
	if len(parts) == 2 {
		info.Message = strings.TrimSpace(parts[1])
	}

	for _, line := range strings.Split(parts[0], "\n") {
		if strings.HasPrefix(line, "tree ") {
			info.Tree = strings.TrimPrefix(line, "tree ")
		} else if strings.HasPrefix(line, "parent ") {
			info.Parents = append(info.Parents, strings.TrimPrefix(line, "parent "))
		} else if strings.HasPrefix(line, "author ") {
			info.Author = strings.TrimPrefix(line, "author ")
		} else if strings.HasPrefix(line, "committer ") {
			info.Committer = strings.TrimPrefix(line, "committer ")
		}
	}

	return info, nil
}

// JSON API: commit log for a branch
func handleAPILog(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	branch := strings.TrimPrefix(r.URL.Path, "/api/log/")
	if branch == "" {
		branch = "master"
	}

	hash, err := getRef(branch)
	if err != nil {
		json.NewEncoder(w).Encode(map[string]string{"error": "branch not found"})
		return
	}

	var commits []CommitInfo
	current := hash
	seen := make(map[string]bool)

	for current != "" && !seen[current] && len(commits) < 100 {
		seen[current] = true
		data, err := readObject(current)
		if err != nil {
			break
		}

		info, err := parseCommitObject(current, data)
		if err != nil {
			break
		}

		commits = append(commits, *info)

		if len(info.Parents) > 0 {
			current = info.Parents[0]
		} else {
			current = ""
		}
	}

	json.NewEncoder(w).Encode(commits)
}

// JSON API: file tree for a commit/tree hash
func handleAPITree(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	hash := strings.TrimPrefix(r.URL.Path, "/api/tree/")
	if hash == "" {
		json.NewEncoder(w).Encode(map[string]string{"error": "hash required"})
		return
	}

	data, err := readObject(hash)
	if err != nil {
		json.NewEncoder(w).Encode(map[string]string{"error": "object not found"})
		return
	}

	content := string(data)
	nullIdx := strings.Index(content, "\x00")
	if nullIdx == -1 {
		json.NewEncoder(w).Encode(map[string]string{"error": "invalid object"})
		return
	}

	body := content[nullIdx+1:]

	type TreeEntry struct {
		Mode string `json:"mode"`
		Type string `json:"type"`
		Hash string `json:"hash"`
		Name string `json:"name"`
	}

	var entries []TreeEntry
	for _, line := range strings.Split(strings.TrimSpace(body), "\n") {
		if line == "" {
			continue
		}
		tabIdx := strings.Index(line, "\t")
		if tabIdx == -1 {
			continue
		}
		meta := line[:tabIdx]
		name := line[tabIdx+1:]

		fields := strings.Fields(meta)
		if len(fields) >= 3 {
			entries = append(entries, TreeEntry{
				Mode: fields[0],
				Type: fields[1],
				Hash: fields[2],
				Name: name,
			})
		}
	}

	json.NewEncoder(w).Encode(entries)
}

// JSON API: blob content by hash
func handleAPIBlob(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Access-Control-Allow-Origin", "*")

	hash := strings.TrimPrefix(r.URL.Path, "/api/blob/")
	if hash == "" {
		http.Error(w, "hash required", http.StatusBadRequest)
		return
	}

	data, err := readObject(hash)
	if err != nil {
		http.Error(w, "object not found", http.StatusNotFound)
		return
	}

	content := string(data)
	nullIdx := strings.Index(content, "\x00")
	if nullIdx == -1 {
		http.Error(w, "invalid object", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Write([]byte(content[nullIdx+1:]))
}

// JSON API: single commit details
func handleAPICommit(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	hash := strings.TrimPrefix(r.URL.Path, "/api/commit/")
	if hash == "" {
		json.NewEncoder(w).Encode(map[string]string{"error": "hash required"})
		return
	}

	data, err := readObject(hash)
	if err != nil {
		json.NewEncoder(w).Encode(map[string]string{"error": "object not found"})
		return
	}

	info, err := parseCommitObject(hash, data)
	if err != nil {
		json.NewEncoder(w).Encode(map[string]string{"error": err.Error()})
		return
	}

	json.NewEncoder(w).Encode(info)
}

// ============================================================================
// EMBEDDED FRONTEND HTML
// ============================================================================
const frontendHTML = `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MyGit — Repository Dashboard</title>
<style>
  :root {
    --bg: #0d1117; --bg2: #161b22; --bg3: #21262d; --border: #30363d;
    --text: #e6edf3; --text2: #8b949e; --accent: #58a6ff; --green: #3fb950;
    --red: #f85149; --orange: #d29922; --purple: #bc8cff;
    --radius: 6px; --shadow: 0 1px 3px rgba(0,0,0,.3);
  }
  * { margin:0; padding:0; box-sizing:border-box; }
  body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
    background: var(--bg); color: var(--text); line-height: 1.5; }
  a { color: var(--accent); text-decoration: none; }
  a:hover { text-decoration: underline; }

  /* Layout */
  .header { background: var(--bg2); border-bottom: 1px solid var(--border); padding: 12px 24px;
    display: flex; align-items: center; gap: 16px; position: sticky; top: 0; z-index: 100; }
  .header h1 { font-size: 20px; font-weight: 600; }
  .header .pill { background: var(--green); color: #000; padding: 2px 10px; border-radius: 12px;
    font-size: 12px; font-weight: 600; }
  .container { max-width: 1200px; margin: 0 auto; padding: 24px; }
  .grid { display: grid; grid-template-columns: 280px 1fr; gap: 24px; }
  @media (max-width: 768px) { .grid { grid-template-columns: 1fr; } }

  /* Cards */
  .card { background: var(--bg2); border: 1px solid var(--border); border-radius: var(--radius);
    overflow: hidden; }
  .card-header { padding: 12px 16px; border-bottom: 1px solid var(--border);
    font-weight: 600; font-size: 14px; display: flex; align-items: center; gap: 8px; }
  .card-body { padding: 0; }

  /* Sidebar */
  .sidebar .branch-item { padding: 8px 16px; cursor: pointer; font-size: 13px;
    border-bottom: 1px solid var(--border); display: flex; align-items: center; gap: 8px;
    transition: background .15s; }
  .sidebar .branch-item:hover { background: var(--bg3); }
  .sidebar .branch-item.active { background: var(--bg3); border-left: 3px solid var(--accent); }
  .sidebar .branch-item .hash { color: var(--text2); font-family: monospace; font-size: 11px; }
  .branch-icon { width: 16px; height: 16px; fill: var(--text2); flex-shrink: 0; }

  /* Tabs */
  .tabs { display: flex; border-bottom: 1px solid var(--border); background: var(--bg2); }
  .tab { padding: 10px 20px; cursor: pointer; font-size: 13px; font-weight: 500;
    border-bottom: 2px solid transparent; color: var(--text2); transition: all .15s; }
  .tab:hover { color: var(--text); }
  .tab.active { color: var(--text); border-bottom-color: var(--accent); }

  /* Commit list */
  .commit-item { padding: 12px 16px; border-bottom: 1px solid var(--border);
    display: flex; gap: 12px; align-items: flex-start; transition: background .15s;
    cursor: pointer; }
  .commit-item:hover { background: var(--bg3); }
  .commit-dot { width: 10px; height: 10px; border-radius: 50%; background: var(--accent);
    flex-shrink: 0; margin-top: 6px; }
  .commit-msg { font-size: 14px; font-weight: 500; }
  .commit-meta { font-size: 12px; color: var(--text2); margin-top: 2px; }
  .commit-hash { font-family: monospace; font-size: 12px; color: var(--accent);
    background: var(--bg3); padding: 1px 6px; border-radius: 3px; }

  /* File tree */
  .file-item { padding: 8px 16px; border-bottom: 1px solid var(--border);
    display: flex; align-items: center; gap: 10px; font-size: 13px; cursor: pointer;
    transition: background .15s; }
  .file-item:hover { background: var(--bg3); }
  .file-icon { font-size: 16px; flex-shrink: 0; }
  .file-hash { font-family: monospace; font-size: 11px; color: var(--text2); margin-left: auto; }

  /* File viewer */
  .file-viewer { background: var(--bg2); border: 1px solid var(--border); border-radius: var(--radius);
    margin-top: 16px; }
  .file-viewer-header { padding: 10px 16px; border-bottom: 1px solid var(--border);
    font-size: 13px; font-weight: 600; display: flex; justify-content: space-between;
    align-items: center; }
  .file-viewer-body { padding: 0; overflow-x: auto; }
  .file-viewer-body pre { padding: 16px; font-family: "SFMono-Regular", Consolas, monospace;
    font-size: 13px; line-height: 1.6; white-space: pre-wrap; word-wrap: break-word; margin: 0; }
  .line-num { display: inline-block; width: 40px; color: var(--text2); text-align: right;
    margin-right: 16px; user-select: none; }

  /* Commit detail */
  .commit-detail { padding: 20px; }
  .commit-detail .field { margin-bottom: 8px; }
  .commit-detail .label { color: var(--text2); font-size: 12px; text-transform: uppercase;
    letter-spacing: 0.5px; }
  .commit-detail .value { font-size: 14px; margin-top: 2px; }
  .commit-detail .value.mono { font-family: monospace; }

  /* Conflict markers */
  .conflict-ours { background: rgba(63,185,80,.15); }
  .conflict-theirs { background: rgba(248,81,73,.15); }
  .conflict-marker { color: var(--orange); font-weight: bold; }

  /* Health badge */
  .health-badge { display: inline-flex; align-items: center; gap: 6px; padding: 4px 12px;
    border-radius: 12px; font-size: 12px; font-weight: 600; }
  .health-badge.ok { background: rgba(63,185,80,.15); color: var(--green); }
  .health-badge.err { background: rgba(248,81,73,.15); color: var(--red); }
  .health-dot { width: 8px; height: 8px; border-radius: 50%; }
  .health-badge.ok .health-dot { background: var(--green); }
  .health-badge.err .health-dot { background: var(--red); }

  /* Empty state */
  .empty-state { padding: 40px 20px; text-align: center; color: var(--text2); }
  .empty-state .icon { font-size: 48px; margin-bottom: 12px; }
  .empty-state p { font-size: 14px; }

  /* Loading spinner */
  .spinner { display: inline-block; width: 20px; height: 20px; border: 2px solid var(--border);
    border-top-color: var(--accent); border-radius: 50%; animation: spin .6s linear infinite; }
  @keyframes spin { to { transform: rotate(360deg); } }

  /* Scrollbar */
  ::-webkit-scrollbar { width: 8px; height: 8px; }
  ::-webkit-scrollbar-track { background: var(--bg); }
  ::-webkit-scrollbar-thumb { background: var(--bg3); border-radius: 4px; }
  ::-webkit-scrollbar-thumb:hover { background: var(--border); }
</style>
</head>
<body>

<div class="header">
  <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
    <circle cx="12" cy="12" r="3"/><path d="M12 3v6m0 6v6"/><path d="M5.636 5.636l4.243 4.243m4.242 4.242l4.243 4.243"/>
    <circle cx="18.364" cy="5.636" r="2"/><circle cx="5.636" cy="18.364" r="2"/>
  </svg>
  <h1>MyGit</h1>
  <span class="pill" id="healthBadge">checking...</span>
  <div style="flex:1"></div>
  <span style="color:var(--text2);font-size:13px" id="serverInfo"></span>
</div>

<div class="container">
  <div class="grid">
    <!-- Sidebar: Branches -->
    <div>
      <div class="card sidebar">
        <div class="card-header">
          <svg class="branch-icon" viewBox="0 0 16 16"><path fill-rule="evenodd" d="M11.75 2.5a.75.75 0 100 1.5.75.75 0 000-1.5zm-2.25.75a2.25 2.25 0 113 2.122V6A2.5 2.5 0 0110 8.5H6a1 1 0 00-1 1v1.128a2.251 2.251 0 11-1.5 0V5.372a2.25 2.25 0 111.5 0v1.836A2.492 2.492 0 016 7h4a1 1 0 001-1v-.628A2.25 2.25 0 019.5 3.25zM4.25 12a.75.75 0 100 1.5.75.75 0 000-1.5zM3.5 3.25a.75.75 0 111.5 0 .75.75 0 01-1.5 0z" fill="currentColor"/></svg>
          Branches <span id="branchCount" style="color:var(--text2);font-weight:400;font-size:12px;margin-left:auto"></span>
        </div>
        <div class="card-body" id="branchList">
          <div class="empty-state"><div class="spinner"></div><p style="margin-top:12px">Loading...</p></div>
        </div>
      </div>
    </div>

    <!-- Main content -->
    <div>
      <div class="tabs">
        <div class="tab active" data-tab="commits" onclick="switchTab('commits')">Commits</div>
        <div class="tab" data-tab="files" onclick="switchTab('files')">Files</div>
        <div class="tab" data-tab="objects" onclick="switchTab('objects')">Object Inspector</div>
      </div>

      <!-- Commits Tab -->
      <div class="card" id="commitsTab">
        <div class="card-body" id="commitList">
          <div class="empty-state"><div class="icon">📜</div><p>Select a branch to view commits</p></div>
        </div>
      </div>

      <!-- Files Tab -->
      <div class="card" id="filesTab" style="display:none">
        <div class="card-body" id="fileTree">
          <div class="empty-state"><div class="icon">📁</div><p>Select a branch to browse files</p></div>
        </div>
      </div>

      <!-- File Viewer (appears below file tree) -->
      <div id="fileViewer" style="display:none"></div>

      <!-- Object Inspector Tab -->
      <div class="card" id="objectsTab" style="display:none">
        <div style="padding:16px">
          <label style="font-size:13px;color:var(--text2)">Object Hash</label>
          <div style="display:flex;gap:8px;margin-top:6px">
            <input id="objHashInput" type="text" placeholder="Enter SHA-1 hash..."
              style="flex:1;padding:8px 12px;background:var(--bg);border:1px solid var(--border);
                border-radius:var(--radius);color:var(--text);font-family:monospace;font-size:13px;outline:none;">
            <button onclick="inspectObject()"
              style="padding:8px 16px;background:var(--accent);color:#fff;border:none;
                border-radius:var(--radius);cursor:pointer;font-size:13px;font-weight:600;">Inspect</button>
          </div>
        </div>
        <div id="objectResult"></div>
      </div>

      <!-- Commit Detail (appears when clicking a commit) -->
      <div id="commitDetail" style="display:none"></div>
    </div>
  </div>
</div>

<script>
const API = '';
let currentBranch = '';
let currentCommits = [];

// Health check
async function checkHealth() {
  const badge = document.getElementById('healthBadge');
  try {
    const res = await fetch(API + '/health');
    if (res.ok) {
      badge.textContent = 'Online';
      badge.className = 'pill';
      badge.style.background = 'var(--green)';
    } else throw new Error();
  } catch {
    badge.textContent = 'Offline';
    badge.className = 'pill';
    badge.style.background = 'var(--red)';
  }
}

// Load branches
async function loadBranches() {
  try {
    const res = await fetch(API + '/api/branches');
    const branches = await res.json();
    const list = document.getElementById('branchList');
    document.getElementById('branchCount').textContent = branches.length;

    if (!branches.length) {
      list.innerHTML = '<div class="empty-state"><div class="icon">🌿</div><p>No branches yet.<br>Push some commits to get started!</p></div>';
      return;
    }

    list.innerHTML = branches.map(b =>
      '<div class="branch-item' + (b.name === currentBranch ? ' active' : '') + '" onclick="selectBranch(\'' + escapeHtml(b.name) + '\')">' +
        '<svg class="branch-icon" viewBox="0 0 16 16"><path fill-rule="evenodd" d="M11.75 2.5a.75.75 0 100 1.5.75.75 0 000-1.5zm-2.25.75a2.25 2.25 0 113 2.122V6A2.5 2.5 0 0110 8.5H6a1 1 0 00-1 1v1.128a2.251 2.251 0 11-1.5 0V5.372a2.25 2.25 0 111.5 0v1.836A2.492 2.492 0 016 7h4a1 1 0 001-1v-.628A2.25 2.25 0 019.5 3.25zM4.25 12a.75.75 0 100 1.5.75.75 0 000-1.5zM3.5 3.25a.75.75 0 111.5 0 .75.75 0 01-1.5 0z" fill="currentColor"/></svg>' +
        '<div><div>' + escapeHtml(b.name) + '</div><div class="hash">' + b.hash.substring(0, 7) + '</div></div>' +
      '</div>'
    ).join('');
  } catch (e) {
    document.getElementById('branchList').innerHTML =
      '<div class="empty-state"><div class="icon">⚠️</div><p>Failed to load branches</p></div>';
  }
}

// Select a branch
async function selectBranch(name) {
  currentBranch = name;
  loadBranches();
  loadCommits(name);
  loadFiles(name);
}

// Load commits for a branch
async function loadCommits(branch) {
  const list = document.getElementById('commitList');
  list.innerHTML = '<div style="padding:20px;text-align:center"><div class="spinner"></div></div>';

  try {
    const res = await fetch(API + '/api/log/' + encodeURIComponent(branch));
    const commits = await res.json();
    currentCommits = commits;

    if (!commits || !commits.length) {
      list.innerHTML = '<div class="empty-state"><div class="icon">📜</div><p>No commits on this branch</p></div>';
      return;
    }

    list.innerHTML = commits.map((c, i) =>
      '<div class="commit-item" onclick="showCommitDetail(\'' + c.hash + '\')">' +
        '<div class="commit-dot" style="background:' + (i === 0 ? 'var(--green)' : 'var(--accent)') + '"></div>' +
        '<div style="flex:1;min-width:0">' +
          '<div class="commit-msg">' + escapeHtml(c.message) + '</div>' +
          '<div class="commit-meta">' +
            '<span class="commit-hash">' + c.hash.substring(0, 7) + '</span> ' +
            escapeHtml(c.author || '') +
            (c.parents && c.parents.length > 1 ? ' <span style="color:var(--purple)">(merge)</span>' : '') +
          '</div>' +
        '</div>' +
      '</div>'
    ).join('');
  } catch {
    list.innerHTML = '<div class="empty-state"><div class="icon">⚠️</div><p>Failed to load commits</p></div>';
  }
}

// Load file tree
async function loadFiles(branch) {
  try {
    const logRes = await fetch(API + '/api/log/' + encodeURIComponent(branch));
    const commits = await logRes.json();
    if (!commits || !commits.length) return;

    const treeHash = commits[0].tree;
    const treeRes = await fetch(API + '/api/tree/' + treeHash);
    const entries = await treeRes.json();

    const tree = document.getElementById('fileTree');
    if (!entries || !entries.length) {
      tree.innerHTML = '<div class="empty-state"><div class="icon">📁</div><p>No files in this tree</p></div>';
      return;
    }

    tree.innerHTML = entries.map(e =>
      '<div class="file-item" onclick="viewFile(\'' + e.hash + '\', \'' + escapeHtml(e.name) + '\')">' +
        '<span class="file-icon">' + (e.type === 'tree' ? '📁' : '📄') + '</span>' +
        '<span>' + escapeHtml(e.name) + '</span>' +
        '<span class="file-hash">' + e.hash.substring(0, 7) + '</span>' +
      '</div>'
    ).join('');
  } catch {
    document.getElementById('fileTree').innerHTML =
      '<div class="empty-state"><div class="icon">⚠️</div><p>Failed to load files</p></div>';
  }
}

// View file content
async function viewFile(hash, name) {
  const viewer = document.getElementById('fileViewer');
  viewer.style.display = 'block';

  viewer.innerHTML =
    '<div class="file-viewer"><div class="file-viewer-header"><span>📄 ' + escapeHtml(name) +
    '</span><span class="commit-hash">' + hash.substring(0, 7) + '</span></div>' +
    '<div class="file-viewer-body"><div style="padding:20px;text-align:center"><div class="spinner"></div></div></div></div>';

  try {
    const res = await fetch(API + '/api/blob/' + hash);
    const text = await res.text();
    const lines = text.split('\n');
    const body = viewer.querySelector('.file-viewer-body');

    let html = '<pre>';
    lines.forEach((line, i) => {
      const num = '<span class="line-num">' + (i + 1) + '</span>';
      const escaped = escapeHtml(line);

      // Highlight conflict markers
      if (line.startsWith('<<<<<<<')) {
        html += num + '<span class="conflict-marker">' + escaped + '</span>\n';
      } else if (line.startsWith('=======')) {
        html += num + '<span class="conflict-marker">' + escaped + '</span>\n';
      } else if (line.startsWith('>>>>>>>')) {
        html += num + '<span class="conflict-marker">' + escaped + '</span>\n';
      } else {
        html += num + escaped + '\n';
      }
    });
    html += '</pre>';
    body.innerHTML = html;
  } catch {
    viewer.querySelector('.file-viewer-body').innerHTML =
      '<div class="empty-state"><p>Failed to load file content</p></div>';
  }
}

// Show commit detail
async function showCommitDetail(hash) {
  const detail = document.getElementById('commitDetail');
  detail.style.display = 'block';

  detail.innerHTML =
    '<div class="card" style="margin-top:16px"><div class="card-header">Commit Details</div>' +
    '<div style="padding:20px;text-align:center"><div class="spinner"></div></div></div>';

  try {
    const res = await fetch(API + '/api/commit/' + hash);
    const c = await res.json();

    let parentsHtml = (c.parents || []).map(p =>
      '<span class="commit-hash" style="cursor:pointer" onclick="showCommitDetail(\'' + p + '\')">' + p.substring(0, 7) + '</span>'
    ).join(' ');
    if (!parentsHtml) parentsHtml = '<span style="color:var(--text2)">none (root commit)</span>';

    // Load tree to show files
    let filesHtml = '';
    try {
      const treeRes = await fetch(API + '/api/tree/' + c.tree);
      const entries = await treeRes.json();
      if (entries && entries.length) {
        filesHtml = '<div class="field" style="margin-top:16px"><div class="label">Files in this commit</div>' +
          entries.map(e =>
            '<div class="file-item" onclick="viewFile(\'' + e.hash + '\', \'' + escapeHtml(e.name) + '\')">' +
              '<span class="file-icon">📄</span>' +
              '<span>' + escapeHtml(e.name) + '</span>' +
              '<span class="file-hash">' + e.hash.substring(0, 7) + '</span>' +
            '</div>'
          ).join('') + '</div>';
      }
    } catch {}

    detail.innerHTML =
      '<div class="card" style="margin-top:16px"><div class="card-header">Commit Details</div><div class="commit-detail">' +
        '<div class="field"><div class="label">Hash</div><div class="value mono">' + c.hash + '</div></div>' +
        '<div class="field"><div class="label">Message</div><div class="value">' + escapeHtml(c.message) + '</div></div>' +
        '<div class="field"><div class="label">Author</div><div class="value">' + escapeHtml(c.author || 'unknown') + '</div></div>' +
        '<div class="field"><div class="label">Tree</div><div class="value mono">' + c.tree + '</div></div>' +
        '<div class="field"><div class="label">Parents</div><div class="value">' + parentsHtml + '</div></div>' +
        filesHtml +
      '</div></div>';
  } catch {
    detail.innerHTML = '<div class="card" style="margin-top:16px"><div class="empty-state"><p>Failed to load commit</p></div></div>';
  }
}

// Inspect arbitrary object
async function inspectObject() {
  const hash = document.getElementById('objHashInput').value.trim();
  if (!hash) return;

  const result = document.getElementById('objectResult');
  result.innerHTML = '<div style="padding:20px;text-align:center"><div class="spinner"></div></div>';

  try {
    const res = await fetch(API + '/object/' + hash);
    if (!res.ok) throw new Error('not found');
    const buf = await res.arrayBuffer();
    const bytes = new Uint8Array(buf);

    // Find null byte separator
    let nullIdx = -1;
    for (let i = 0; i < bytes.length; i++) {
      if (bytes[i] === 0) { nullIdx = i; break; }
    }

    const decoder = new TextDecoder();
    const header = nullIdx >= 0 ? decoder.decode(bytes.slice(0, nullIdx)) : '(unknown)';
    const body = nullIdx >= 0 ? decoder.decode(bytes.slice(nullIdx + 1)) : decoder.decode(bytes);

    result.innerHTML =
      '<div style="padding:16px;border-top:1px solid var(--border)">' +
        '<div style="margin-bottom:8px"><span style="color:var(--text2);font-size:12px">HEADER</span>' +
          '<div class="commit-hash" style="display:inline-block;margin-left:8px">' + escapeHtml(header) + '</div></div>' +
        '<div><span style="color:var(--text2);font-size:12px">BODY</span>' +
          '<pre style="margin-top:6px;padding:12px;background:var(--bg);border-radius:var(--radius);font-size:13px;overflow-x:auto">' +
            escapeHtml(body) + '</pre></div></div>';
  } catch {
    result.innerHTML =
      '<div style="padding:16px;text-align:center;color:var(--red)">Object not found or error fetching</div>';
  }
}

// Tab switching
function switchTab(tab) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelector('[data-tab="' + tab + '"]').classList.add('active');
  document.getElementById('commitsTab').style.display = tab === 'commits' ? '' : 'none';
  document.getElementById('filesTab').style.display = tab === 'files' ? '' : 'none';
  document.getElementById('objectsTab').style.display = tab === 'objects' ? '' : 'none';
  document.getElementById('fileViewer').style.display = 'none';
  document.getElementById('commitDetail').style.display = 'none';
}

// Utility
function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

// Init
checkHealth();
loadBranches();
document.getElementById('serverInfo').textContent = 'Server: ' + location.host;
</script>
</body>
</html>` + ""
