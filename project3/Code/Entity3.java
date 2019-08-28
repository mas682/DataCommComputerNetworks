public class Entity3 extends Entity
{
    // Perform any necessary initialization in the constructor
    public Entity3()
    {
        System.out.println("Initializing entity 3 at time: " + NetworkSimulator.time);
        // used to initialize distance table
        for(int x = 0; x < 4; x++)
        {
            for(int y = 0; y < 4; y++)
            {
                distanceTable[x][y] = 999;
            }
        }
        distanceTable[3][0] = 7;
        distanceTable[3][1] = 999;
        distanceTable[3][2] = 2;
        distanceTable[3][3] = 0;
        int i = 0;
        // send initial distance vectors to neighbors
        while(i < 4)
        {
            // 1 cannot send vector to itself or 3
            if(i == 1 || i == 3)
            {
                i++;
                continue;
            }
            Packet packet = new Packet(3, i, distanceTable[3]);
            System.out.println("Sending first distance vector from 3 to " + i + " at " + NetworkSimulator.time);
            NetworkSimulator.toLayer2(packet);
            i++;
        }
        System.out.println();
    }

    // Handle updates when a packet is received.  Students will need to call
    // NetworkSimulator.toLayer2() with new packets based upon what they
    // send to update.  Be careful to construct the source and destination of
    // the packet correctly.  Read the warning in NetworkSimulator.java for more
    // details.
    public void update(Packet p)
    {
        System.out.println("Packet received in 3 via " + p.getSource() + " at time: " + NetworkSimulator.time);
        int myDistanceVector [] = distanceTable[3];
        int neighbors [] = NetworkSimulator.cost[3];
        int i = 0;
        int oldDistanceVector[] = new int[4];
        while(i < 4)
        {
            oldDistanceVector[i] = myDistanceVector[i];
            i++;
        }
        i = 0;
        while(i < 4)
        {
            distanceTable[p.getSource()][i] = p.getMincost(i);
            i++;
        }
        i = 0;              // i = 1 as 0 does not need to check itself
        boolean vectorChanged = false;
        while(i < 4)
        {
            //int currentDistance = myDistanceVector[i];
            int directDistance = neighbors[i];
            int tempDistance = 999;
            int newDistance = 0;
            for(int x = 0; x <4; x++)
            {
                if(x == 3)
                {
                    continue;
                }
                newDistance = distanceTable[x][i] + neighbors[x];
                if(newDistance < tempDistance)
                {
                    tempDistance = newDistance;
                }
            }
            if(directDistance < tempDistance)
            {
                myDistanceVector[i] = directDistance;
            }
            else
            {
                myDistanceVector[i] = tempDistance;
            }
            i++;
        }
        i = 0;
        while(i < 4)
        {
            if(myDistanceVector[i] != oldDistanceVector[i])
            {
                vectorChanged = true;
                break;
            }
            i++;
        }
        if(vectorChanged)
        {
            i = 0;
            while(i < 4)
            {
                distanceTable[3][i] = myDistanceVector[i];
                i++;
            }
            // if the vector was changed, send it to neighbors
            System.out.println("Distance vector changed in 3");
            toStringVector();
            printDT();
            i = 0;
            while(i < 4)
            {
                // 1 cannot send vector to itself or 3
                if(i == 1 || i == 3)
                {
                    i++;
                    continue;
                }
                Packet packet = new Packet(3, i, distanceTable[3]);
                System.out.println("Sending distance vector from 3 to " + i + " at: " + NetworkSimulator.time);
                NetworkSimulator.toLayer2(packet);
                i++;
            }
        }
        else
        {
            System.out.println("The distance vector in 3 was not updated");
            printDT();
        }
        System.out.println();
    }

    public void linkCostChangeHandler(int whichLink, int newCost)
    {
    }

    public void printDT()
    {
        System.out.println();
        System.out.println("         via");
        System.out.println(" D3 |   0   2");
        System.out.println("----+--------");
        for (int i = 0; i < NetworkSimulator.NUMENTITIES; i++)
        {
            if (i == 3)
            {
                continue;
            }

            System.out.print("   " + i + "|");
            for (int j = 0; j < NetworkSimulator.NUMENTITIES; j += 2)
            {

                if (distanceTable[i][j] < 10)
                {
                    System.out.print("   ");
                }
                else if (distanceTable[i][j] < 100)
                {
                    System.out.print("  ");
                }
                else
                {
                    System.out.print(" ");
                }

                System.out.print(distanceTable[i][j]);
            }
            System.out.println();
        }
    }

    public void toStringVector()
    {
        System.out.println("Updated distance vector for 3:");
        System.out.println("[" + distanceTable[3][0] + ", " + distanceTable[3][1] + ", " + distanceTable[3][2] + ", " + distanceTable[3][3] + "]");
    }
}
