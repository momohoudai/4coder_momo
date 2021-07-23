using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;




// TODO(Momo):
// [ ] Map generator
// [ ] Handle case where there are no neighbours for selector
// [ ] Review selection system? (maybe don't use a loose int just to keep track of current neighbour index? maybe use a neighbour index + node index?) 
// [ ] Map generation tracker (to check for invalid handles? Might not need this but good to have) 

public enum MapNodeTypes
{
    START,
    FIGHT,
    EVENT,
    CAMP,
    BOSS
}

// Negative index is invalid
public struct MapNodeHandle
{
    public int index;
    public MapNodeHandle(int index) {
        this.index = index;
    }
    public static MapNodeHandle Invalid = new MapNodeHandle(-1); 
}

public class MapNode
{
    // NOTE(Momo): Used for map traversal
    public List<MapNodeHandle> neighbours = new List<MapNodeHandle>();
    public bool explored; 
    
    // NOTE(Momo): Game related data. Technically better to
    // seperate these stuff to another class or even just let it
    // hold a base class/object class since we are pOOPing.
    public MapNodeTypes type;
    public Vector2 position;
    public string name;
}


// NOTE(Momo): Try not to hold direct references to MapNode anywhere in the game to prevent
// garbage collection issues. Use MapNodeHandle instead. 
// 
// TODO(Momo) We might want to have some kind of generation count whenever we rege the map and add that
// into into MapNodeHandle. This way whenever we regen the map, we have some way of identifying if a MapNodeHandle is 
// from an older generation of the map. 
//
public class Map : MonoBehaviour
{
    [InspectorName("Node Prefab")]
        public GameObject node_prefab;
    
    [InspectorName("Selector Prefab")]
        public GameObject selector_prefab;
    
    private delegate void TraverseCallback(MapNode node);
    private Queue<MapNode> bfs_queue = new Queue<MapNode>();
    
    
    private MapNodeHandle current_node_handle; // TODO: store in player?
    private int selected_neighbour_index;
    private List<MapNode> master_node_list = new List<MapNode>();
    
    // Map Node management API 
    void TraverseMapNodesBFS(TraverseCallback callback) {
        if (master_node_list.Count == 0)
            return;
        
        // TODO: Set all node.explored to false
        var root = this.master_node_list[0];
        
        bfs_queue.Clear();
        bfs_queue.Enqueue(root);
        root.explored = true;
        
        while(bfs_queue.Count > 0) {
            MapNode node = bfs_queue.Dequeue();
            callback(node);
            foreach (MapNodeHandle neighbour_handle in node.neighbours)
            {
                MapNode neighbour = this.GetMapNode(neighbour_handle); 
                if (!neighbour.explored)
                {
                    neighbour.explored = true;
                    bfs_queue.Enqueue(neighbour);
                }
            }
        }       
    }
    
    
    MapNodeHandle AddMapNodeToMasterList(string name, MapNodeTypes type, Vector3 position)
    {
        MapNode node = new MapNode();
        node.name = name;
        node.type = type;
        node.position = position;
        
        this.master_node_list.Add(node);
        return new MapNodeHandle(this.master_node_list.Count - 1);
    }
    
    MapNode GetMapNode(MapNodeHandle handle)
    {
        
        if (handle.index < 0 && handle.index >= this.master_node_list.Count)
            return null;
        return this.master_node_list[handle.index];
    }
    
    bool AddNeighbourToMapNode(MapNodeHandle from, MapNodeHandle to)
    {
        MapNode from_node = GetMapNode(from);
        MapNode to_node = GetMapNode(to);
        if (from_node == null || to_node == null) {
            return false;
        }
        from_node.neighbours.Add(to);
        return true;
    }
    
    
    void Start()
    {
        
        // NOTE(Momo): for now, we just generate something hard coded
        // TODO(Momo): Do some real generation
        {
            //        s
            //       / \
            //      f   f
            //     /   / \ 
            //    e   c   f
            //    \   |   /
            //     \  |  /
            //        b    
            
            Vector2 offset = new Vector2(200, 200);
            Vector2 start = new Vector2(100, 100);
            Vector2 current = start;   
            
            // layer 1
            MapNodeHandle start_1 = AddMapNodeToMasterList("start_1", MapNodeTypes.START, current);
            
            // layer2
            current.y = start.y;
            current.x += offset.x;
            
            MapNodeHandle fight_1 = AddMapNodeToMasterList("fight_1", MapNodeTypes.FIGHT, current);
            current.y += offset.y;
            
            
            MapNodeHandle fight_2 = AddMapNodeToMasterList("fight_2", MapNodeTypes.FIGHT, current);
            current.y += offset.y;
            
            // layer 3
            current.y = start.y;
            current.x += offset.x; 
            
            
            MapNodeHandle fight_3 = AddMapNodeToMasterList("fight_3", MapNodeTypes.FIGHT, current);
            current.y += offset.y;
            
            MapNodeHandle event_1 = AddMapNodeToMasterList("event_1", MapNodeTypes.EVENT, current);
            current.y += offset.y;
            
            MapNodeHandle camp_1 = AddMapNodeToMasterList("camp_1", MapNodeTypes.CAMP, current);
            current.y += offset.y;
            
            // layer 4
            current.y = start.y;
            current.x += offset.x;
            
            MapNodeHandle boss_1 = AddMapNodeToMasterList("boss_1", MapNodeTypes.BOSS, current);
            
            // NOTE(Momo): Add neighbours 
            // Do we design this bottom up or top down?  
            
            AddNeighbourToMapNode(event_1, boss_1);
            AddNeighbourToMapNode(camp_1, boss_1);
            AddNeighbourToMapNode(fight_3, boss_1);
            AddNeighbourToMapNode(fight_1, event_1);
            AddNeighbourToMapNode(fight_2, camp_1);
            AddNeighbourToMapNode(fight_2, fight_3);
            AddNeighbourToMapNode(start_1, fight_1);
            AddNeighbourToMapNode(start_1, fight_2);
            
            // Test
            TraverseMapNodesBFS((MapNode node) =>
                                {
                                    Debug.Log(node.name);
                                    var go = Instantiate(node_prefab, this.transform);
                                    go.name = node.name;
                                    go.transform.position = node.position;
                                    var img = go.GetComponent<Image>();
                                    
                                    switch(node.type)
                                    {
                                        case MapNodeTypes.BOSS:
                                        img.color = Color.red;
                                        break;
                                        case MapNodeTypes.CAMP:
                                        img.color = Color.green;
                                        break;
                                        case MapNodeTypes.EVENT:
                                        img.color = Color.yellow;
                                        break;
                                        case MapNodeTypes.START:
                                        img.color = Color.blue;
                                        break;
                                        case MapNodeTypes.FIGHT:
                                        img.color = Color.magenta;
                                        break;
                                    }
                                    
                                });
            
        }
        
        // NOTE(Momo): LOL why is this possible? It's so stupid.
        this.selector_prefab = Instantiate(this.selector_prefab, this.transform);
        
        
        // TODO: Find the node where the player is.
        // We probably have to decide how we are storing player's progress 
        // in the game and then use that to do this.
        // i.e. the code below is just a dummy code.
        this.current_node_handle = new MapNodeHandle(0);
        var current_node = this.GetMapNode(this.current_node_handle); 
        if (current_node == null) {
            return;
        }
        
        // Determine default selected node based on current node
        if (current_node.neighbours.Count > 0) {
            this.selected_neighbour_index = 0; 
        }
        else {
            this.selected_neighbour_index = -1;
        }
        
        UpdateSelector();
        
    }
    
    void UpdateSelector()
    { 
        // TODO: I feel like this code is wrong?
        // I mean, it works but think what it does is full of holes
        MapNode node = this.GetMapNode(this.current_node_handle);
        if (node == null || node.neighbours.Count <= 0)
        {
            selector_prefab.SetActive(false);
        }
        else
        {
            MapNode neighbourNode = this.GetMapNode(node.neighbours[this.selected_neighbour_index]);
            selector_prefab.transform.position = neighbourNode.position;
            selector_prefab.SetActive(true);
        }
        
    }
    
    void Update()
    {
        // NOTE(Momo): Cycle through neighbours
        // Same logic as Eden. Just need to cycle though neighbours.
        // TODO: Change to better input system, if it exists. 
        
        if (Input.GetKeyDown("up")) {
            MapNode current_node = this.GetMapNode(this.current_node_handle);
            if(current_node != null)
            {
                ++this.selected_neighbour_index;
                if(this.selected_neighbour_index >= current_node.neighbours.Count) {
                    this.selected_neighbour_index = current_node.neighbours.Count - 1;
                } 
            }
            UpdateSelector();
        }
        else if (Input.GetKeyDown("down"))
        {
            --this.selected_neighbour_index;
            if (this.selected_neighbour_index <= 0) {
                this.selected_neighbour_index = 0;
            }
            UpdateSelector();
        }
        
    }
    
}
