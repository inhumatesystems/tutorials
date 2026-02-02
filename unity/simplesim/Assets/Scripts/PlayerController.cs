using System.Collections;
using System.Collections.Generic;
using UnityEngine;



public class PlayerController : MonoBehaviour
{
    public float force = 1000f;
    public float torque = 50f;
    public float cubeSpawnDist = 3f;
    public GameObject boxPrefab;

    Rigidbody rb;

    // Start is called before the first frame update
    void Start()
    {
        rb = GetComponent<Rigidbody>();
    }

    // Update is called once per frame
    void Update()
    {
        // Instantiate boxprefab when pressing B, cubeSpawnDist units from the player
        if (Input.GetKeyDown(KeyCode.B))
        {
            Instantiate(boxPrefab, transform.position + transform.forward * cubeSpawnDist, Quaternion.identity);
        }
    }

     void FixedUpdate()
     {
        // Forward / Backward movement
        if (Input.GetKey(KeyCode.W))
        {
            rb.AddForce(transform.forward * force, ForceMode.Force);
        }

        if (Input.GetKey(KeyCode.S))
        {
            rb.AddForce(-transform.forward * force, ForceMode.Force);
        }

        if (Input.GetKey(KeyCode.A))
        {
            rb.AddTorque(0, -torque, 0);
        }

        if (Input.GetKey(KeyCode.D))
        {
            rb.AddTorque(0, torque, 0);
        }
     }
}
