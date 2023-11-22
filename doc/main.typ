#import "template.typ": *

// Take a look at the file `template.typ` in the file panel
// to customize this template and discover how it works.
#show: project.with(
  title: "Capstone Project Design",
  authors: (
    "Capstone Group Xi",
  ),
)

// We generated the example code below so you can see how
// your document will look. Go ahead and replace it with
// your own content!

= Clear Objective/Problem Statement
`The design should begin with a clearly articulated objective or problem statement. It should outline what the project aims to achieve or the specific problem it seeks to address.`

The primary objective of this project is to design and implement a robust, scalable, and efficient cluster management system inspired by Google's Borg. Our system will aim to optimize resource allocation, workload scalability, fault tolerance and operational efficiency in managing large-scale, distributed computing tasks across a data center.

Problem Statement:


= Relevance
`The design should emphasize how the project relates to distributed systems, either by leveraging principles from the course or addressing challenges specific to distributed computing.`

= Technical Depth and Complexity
`The proposed design should reflect an appropriate level of technical depth, showcasing understanding and thoughtful application of distributed system concepts.`

1. The primary language for the project is C++, a statically typed and compiled language with rich typing support. This language requires more understanding of the lower level machine and thread scheduling

2. The project used gRPC and ProtoBuf for rpcs between different nodes, which provides more structured messaging schemes between different nodes. However, it takes more time to understand the semantics and feature of ProtoBuf

3. Bazel is used as the project manager to glue ProtoBuf, gRPC and C++ together. However, Bazel itself is harder to use compared to package manager of Python or Java. However, it is customizable to easily support integration of gRPC and ProtoBuf

4. Master nodes use Raft as consensus algorithm for replication. Thus, once a task is submitted, it would be replicated to raft group, and user can always expect the task to have an result as long as no more than half of Raft group fail

5. Master nodes is responsible for worker scheduling, and workers can be added dynamically and contact master through heartbeat. In this case, scaling the system would be really easy

= Feasibility and Scope
`The project design should be feasible given the time, resources, and skills available. It should also have a defined scope, making it clear what's in and out of the project's purview.`

= Architecture and Design Principles
`The design should detail the architectural choices and design principles underpinning the project. This could include decisions about data replication, fault tolerance, scalability, and more.`

= Methodology and Implementation Strategy
`The design should outline the methods, tools, and technologies the student envisions using, as well as a general strategy or roadmap for implementation.`

= Evaluation and Metrics
`The design should specify how the project's success will be measured, what metrics will be used, and how the system's performance or results will be evaluated.`

= Risk Management and Contingency Planning
`The design should identify potential risks or challenges that could arise during the project and provide contingency plans to address them.`

= Background
`The design should reference relevant literature, previous work in the domain, or foundational papers that inform the project's direction.`

= Clarity and Organization
`The design document should be well-organized, free from ambiguity, and presented in a logical manner, making it easy for readers to understand.`